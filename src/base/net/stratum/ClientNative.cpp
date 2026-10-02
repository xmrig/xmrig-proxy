/* XMRig
 * Native multi-algorithm stratum compatibility support.
 */

#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

#include "base/net/stratum/Client.h"
#include "3rdparty/rapidjson/document.h"
#include "3rdparty/rapidjson/stringbuffer.h"
#include "3rdparty/rapidjson/writer.h"
#include "base/io/json/Json.h"
#include "base/io/json/JsonRequest.h"
#include "base/io/log/Log.h"
#include "base/kernel/interfaces/IClientListener.h"
#include "base/net/stratum/Job.h"
#include "base/net/stratum/NativeTarget.h"
#include "net/JobResult.h"


namespace {


bool valueToString(const rapidjson::Value &value, xmrig::String &out)
{
    if (value.IsString()) {
        out = xmrig::String(value.GetString(), value.GetStringLength());
        return !out.isEmpty();
    }

    char buf[64] = {};
    if (value.IsInt64()) {
        snprintf(buf, sizeof(buf), "%" PRId64, value.GetInt64());
    }
    else if (value.IsUint64()) {
        snprintf(buf, sizeof(buf), "%" PRIu64, value.GetUint64());
    }
    else {
        return false;
    }

    out = xmrig::String(buf, strlen(buf));
    return true;
}


bool unsignedInteger(const rapidjson::Value &value, uint64_t &out)
{
    if (value.IsUint64()) {
        out = value.GetUint64();
        return true;
    }

    if (value.IsInt64() && value.GetInt64() >= 0) {
        out = static_cast<uint64_t>(value.GetInt64());
        return true;
    }

    return false;
}


bool positiveNumber(const rapidjson::Value &value, double &out)
{
    if (!value.IsNumber()) {
        return false;
    }

    out = value.GetDouble();
    return std::isfinite(out) && out > 0.0;
}


bool isHex(const char value)
{
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') ||
           (value >= 'A' && value <= 'F');
}


bool strictHex(const char *value, size_t length)
{
    if (!value || (length & 1) != 0 || strlen(value) != length) {
        return false;
    }

    for (size_t i = 0; i < length; ++i) {
        if (!isHex(value[i])) {
            return false;
        }
    }

    return true;
}


bool parsePrefix(const rapidjson::Value &prefixValue, const rapidjson::Value &remainingValue,
                 xmrig::String &prefix, uint32_t &remainingBytes)
{
    if (!prefixValue.IsString()) {
        return false;
    }

    const char *value = prefixValue.GetString();
    const size_t length = prefixValue.GetStringLength();
    if (length == 0 || (length & 1) != 0 || length > 14 || !strictHex(value, length)) {
        return false;
    }

    uint64_t remaining = 0;
    if (!unsignedInteger(remainingValue, remaining) || remaining == 0 || remaining > 8 ||
        (length / 2) + remaining != 8) {
        return false;
    }

    prefix = xmrig::String(value, length);
    remainingBytes = static_cast<uint32_t>(remaining);
    return true;
}


uint64_t difficultyFromTarget(const char *value)
{
    return xmrig::NativeTarget::difficulty(value, true);
}


uint64_t nativeDifficulty(double value, const xmrig::Algorithm &algorithm)
{
    if (!std::isfinite(value) || value <= 0.0) {
        return 0;
    }

    long double difficulty = value;
    if (algorithm == xmrig::Algorithm::ETHASH || algorithm == xmrig::Algorithm::ETCHASH) {
        difficulty *= 4294967296.0L;
    }

    if (difficulty >= static_cast<long double>(std::numeric_limits<uint64_t>::max())) {
        return std::numeric_limits<uint64_t>::max();
    }

    const long double rounded = std::floor(difficulty + 0.5L);
    return rounded < 1.0L ? 1 : static_cast<uint64_t>(rounded);
}


bool decimalTarget(const char *value, xmrig::String &hex)
{
    xmrig::NativeTarget::UInt256 target = {};
    if (!xmrig::NativeTarget::strictDecimalParse(value, target)) {
        return false;
    }

    bool nonzero = false;
    for (const uint8_t byte : target) {
        nonzero = nonzero || byte != 0;
    }
    if (!nonzero) {
        return false;
    }

    char output[65] = {};
    if (!xmrig::NativeTarget::toHex64(target, output, sizeof(output))) {
        return false;
    }

    hex = xmrig::String(output, 64);
    return true;
}


bool appendNonceSpace(const char *header, std::string &blob)
{
    if (!strictHex(header, 64)) {
        return false;
    }

    blob.assign(header, 64);
    blob.append(16, '0');
    return true;
}


xmrig::String serializeJSON(const rapidjson::Value &value)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);

    return xmrig::String(buffer.GetString(), buffer.GetSize());
}


bool isNativeArrayAlgorithm(const xmrig::Algorithm &algorithm)
{
    return algorithm.family() == xmrig::Algorithm::KAWPOW ||
           algorithm == xmrig::Algorithm::ETHASH || algorithm == xmrig::Algorithm::ETCHASH ||
           algorithm == xmrig::Algorithm::AUTOLYKOS2;
}


bool taggedAlgorithm(const rapidjson::Value &message, xmrig::Algorithm &algorithm, bool &tagged)
{
    tagged = false;
    const char *name = xmrig::Json::getString(message, "algo");
    if (!name) {
        return true;
    }

    algorithm = xmrig::Algorithm(name);
    if (!algorithm.isValid()) {
        return false;
    }

    tagged = true;
    return true;
}


bool controlMatches(const xmrig::String &controlAlgorithm, const xmrig::Algorithm &algorithm)
{
    return !controlAlgorithm.isNull() &&
           (controlAlgorithm.isEmpty() || controlAlgorithm == algorithm.name());
}


uint64_t controlDifficulty(const xmrig::String &control, const xmrig::Algorithm &algorithm)
{
    if (control.isNull() || control.isEmpty()) {
        return 0;
    }

    rapidjson::Document document;
    if (document.Parse(control.data(), control.size()).HasParseError() || !document.IsObject()) {
        return 0;
    }

    const char *method = xmrig::Json::getString(document, "method");
    const rapidjson::Value &params = xmrig::Json::getValue(document, "params");
    if (!method || !params.IsArray() || params.Size() != 1) {
        return 0;
    }

    if (strcmp(method, "mining.set_target") == 0 && params[0].IsString()) {
        return difficultyFromTarget(params[0].GetString());
    }

    if (strcmp(method, "mining.set_difficulty") == 0 && params[0].IsNumber()) {
        return nativeDifficulty(params[0].GetDouble(), algorithm);
    }

    return 0;
}


} // namespace


void xmrig::Client::subscribeNative()
{
    if (!m_nativeRequested || m_nativeSubscribed || m_state != ConnectedState) {
        return;
    }

    using namespace rapidjson;

    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();
    Value params(kArrayType);
    params.PushBack(StringRef(m_agent), allocator);

    JsonRequest::create(doc, m_sequence, "mining.subscribe", params);
    send(doc, [this](const Value &result, bool success, uint64_t) {
        if (success) {
            parseNativeSubscribe(result);
        }
    });
}


void xmrig::Client::parseNativeSubscribe(const rapidjson::Value &result)
{
    if (!result.IsArray() || result.Size() < 3) {
        return;
    }

    uint32_t remaining = 0;
    xmrig::String prefix;
    if (!parsePrefix(result[1], result[2], prefix, remaining)) {
        return;
    }

    if (!setNativePrefix(prefix.data(), remaining, &result)) {
        return;
    }

    m_nativeSubscribed = true;
}


bool xmrig::Client::setNativePrefix(const char *prefix, uint32_t remainingBytes,
                                     const rapidjson::Value *message)
{
    if (!prefix || !*prefix || (strlen(prefix) & 1) != 0 || strlen(prefix) > 14 ||
        !strictHex(prefix, strlen(prefix)) || remainingBytes == 0 || remainingBytes > 8 ||
        strlen(prefix) / 2 + remainingBytes != 8) {
        return false;
    }

    const bool changed = m_nativePrefix.isNull() || !m_nativePrefix.isEqual(prefix) ||
                         m_nativeNonceSize != remainingBytes;
    m_nativePrefix = prefix;
    m_nativeNonceSize = remainingBytes;

    if (changed && message && m_listener && m_job.isValid() && !isNativeArrayAlgorithm(m_job.algorithm())) {
        Job snapshot = m_job;
        snapshot.setNativePrefix(m_nativePrefix.data());
        m_job = std::move(snapshot);
        m_nativePrefixUpdated = true;
        m_listener->onJobReceived(this, m_job, *message);
    }

    return true;
}


void xmrig::Client::parseNativeControl(const rapidjson::Value &message)
{
    const rapidjson::Value &params = Json::getValue(message, "params");
    const char *method = Json::getString(message, "method");
    if (!method || !params.IsArray()) {
        return;
    }

    Algorithm tagged;
    bool isTagged = false;
    if (!taggedAlgorithm(message, tagged, isTagged)) {
        return;
    }

    if (strcmp(method, "mining.set_extranonce") == 0) {
        if (params.Size() != 2) {
            return;
        }

        uint64_t remaining = 0;
        if (!unsignedInteger(params[1], remaining) || remaining > UINT32_MAX) {
            return;
        }

        if (!setNativePrefix(params[0].IsString() ? params[0].GetString() : nullptr,
                             static_cast<uint32_t>(remaining), &message)) {
            return;
        }

        m_nativeSubscribed = true;
        return;
    }

    if (strcmp(method, "mining.set_target") == 0) {
        if (params.Size() != 1 || !params[0].IsString() || difficultyFromTarget(params[0].GetString()) == 0) {
            return;
        }

        m_nativeTarget = params[0].GetString();
    }
    else if (strcmp(method, "mining.set_difficulty") == 0) {
        double value = 0.0;
        if (params.Size() != 1 || !positiveNumber(params[0], value)) {
            return;
        }

        m_nativeTarget = nullptr;
    }
    else {
        return;
    }

    m_nativeControl = serializeJSON(message);
    m_nativeControlAlgo = isTagged ? tagged.name() : "";
}


bool xmrig::Client::parseNativeNotify(const rapidjson::Value &message)
{
    const rapidjson::Value &params = Json::getValue(message, "params");
    const char *algoName = Json::getString(message, "algo");
    if (!algoName && params.IsObject()) {
        algoName = Json::getString(params, "algo");
    }
    Algorithm algorithm(algoName);
    if (!algorithm.isValid()) {
        algorithm = m_pearlLogin && params.IsObject() ? Algorithm(Algorithm::PEARLHASH) : m_pool.algorithm();
    }

    if (algorithm == Algorithm::PEARLHASH && params.IsObject()) {
        int code = -1;
        if (params.HasMember("algo")) {
            return parseJob(params, &code, &message);
        }

        rapidjson::Document tagged;
        tagged.CopyFrom(params, tagged.GetAllocator());
        tagged.AddMember("algo", rapidjson::StringRef(Algorithm::kPEARLHASH), tagged.GetAllocator());

        return parseJob(tagged, &code, &message);
    }

    if (!params.IsArray()) {
        return false;
    }
    if (!isNativeArrayAlgorithm(algorithm) || params.Empty()) {
        return false;
    }

    if (m_nativePrefix.isNull() || m_nativePrefix.isEmpty() || m_nativeNonceSize == 0) {
        return false;
    }

    String id;
    if (!valueToString(params[0], id)) {
        return false;
    }

    const rapidjson::Value *header = nullptr;
    const rapidjson::Value *target = nullptr;
    String fullTarget;
    uint64_t height = 0;
    uint64_t diff = 0;

    if (algorithm.family() == Algorithm::KAWPOW) {
        if (params.Size() != 7 || !params[1].IsString() || !params[2].IsString() ||
            !params[3].IsString() || !params[4].IsBool() || !unsignedInteger(params[5], height)) {
            return false;
        }
        if (!strictHex(params[1].GetString(), 64) || !strictHex(params[2].GetString(), 64) ||
            difficultyFromTarget(params[3].GetString()) == 0) {
            return false;
        }
        if (params[6].IsString()) {
            if (!strictHex(params[6].GetString(), 8)) {
                return false;
            }
        }
        else {
            uint64_t nbits = 0;
            if (!unsignedInteger(params[6], nbits) || nbits > UINT32_MAX) {
                return false;
            }
        }

        header = &params[1];
        target = &params[3];
        diff = difficultyFromTarget(target->GetString());
    }
    else if (algorithm == Algorithm::ETHASH || algorithm == Algorithm::ETCHASH) {
        if (params.Size() != 4 || !params[1].IsString() || !params[2].IsString() || !params[3].IsBool() ||
            !strictHex(params[1].GetString(), 64) || !strictHex(params[2].GetString(), 64)) {
            return false;
        }

        header = &params[2];
    }
    else {
        uint64_t network = 0;
        if (params.Size() != 9 || !params[1].IsNumber() || !params[2].IsString() ||
            !params[3].IsString() || !params[4].IsString() || !params[5].IsNumber() ||
            !params[6].IsString() || !params[7].IsString() || !params[8].IsBool() ||
            !strictHex(params[2].GetString(), 64) || !decimalTarget(params[6].GetString(), fullTarget) ||
            !unsignedInteger(params[1], height) || !unsignedInteger(params[5], network)) {
            return false;
        }

        header = &params[2];
        diff = difficultyFromTarget(fullTarget.data());
    }

    std::string blob;
    if (!appendNonceSpace(header->GetString(), blob)) {
        return false;
    }

    Job job(has<EXT_NICEHASH>(), algorithm, m_rpcId);
    job.setNativeNonce(32, 8);
    job.setNativePrefix(m_nativePrefix.data());
    if (!job.setId(id.data()) || !job.setBlob(blob.c_str())) {
        return false;
    }

    if (target && !job.setNativeTarget(target->GetString(), true)) {
        return false;
    }
    if (!fullTarget.isNull() && !job.setNativeTarget(fullTarget.data(), true)) {
        return false;
    }

    const bool hasControl = controlMatches(m_nativeControlAlgo, algorithm);
    if (hasControl && m_nativeControlAlgo.isEmpty()) {
        m_nativeControlAlgo = algorithm.name();
    }

    if (!target && fullTarget.isNull() && hasControl) {
        if (!m_nativeTarget.isNull()) {
            diff = difficultyFromTarget(m_nativeTarget.data());
            if (!diff || !job.setNativeTarget(m_nativeTarget.data(), true)) {
                return false;
            }
        }
        else {
            diff = controlDifficulty(m_nativeControl, algorithm);
        }
    }

    if (!diff || !job.setNativePayload(message)) {
        return false;
    }

    job.setDiff(diff);
    job.setHeight(height);
    if (!verifyAlgorithm(job.algorithm(), algorithm.name())) {
        return false;
    }
    if (hasControl && !m_nativeControl.isNull()) {
        job.setNativeControl(m_nativeControl.data());
    }

    m_job.setClientId(m_rpcId);
    if (m_job != job) {
        m_nativePrefixUpdated = false;
        m_jobs++;
        m_job = std::move(job);
        return true;
    }

    if (m_jobs == 0) {
        return false;
    }

    if (!isQuiet()) {
        LOG_WARN("%s " YELLOW("duplicate native job received, reconnect"), tag());
    }

    close();
    return false;
}


bool xmrig::Client::parseNativeObjectJob(const rapidjson::Value &message)
{
    const rapidjson::Value &params = Json::getValue(message, "params");
    if (!params.IsObject()) {
        return false;
    }

    if (m_nativePrefix.isNull() || m_nativePrefix.isEmpty() || m_nativeNonceSize == 0) {
        return false;
    }

    uint64_t nonceBytes = 0;
    if (!params.HasMember("noncebytes") || !unsignedInteger(params["noncebytes"], nonceBytes) ||
        nonceBytes == 0 || nonceBytes > 8) {
        return false;
    }

    uint64_t nonceOffset = 0;
    if (params.HasMember("nonceoffset") && !unsignedInteger(params["nonceoffset"], nonceOffset)) {
        return false;
    }
    if (nonceOffset > Job::kMaxBlobSize - nonceBytes) {
        return false;
    }

    int code = -1;
    return parseJob(params, &code, &message);
}


void xmrig::Client::setNativeMetadata(const rapidjson::Value &result, bool notifyCurrentJob)
{
    const rapidjson::Value &prefix = Json::getValue(result, "extra_nonce");
    if (!prefix.IsString()) {
        return;
    }

    uint64_t remaining = 0;
    const rapidjson::Value &size = Json::getValue(result, "extra_nonce_size");
    if (!size.IsNull()) {
        if (!unsignedInteger(size, remaining) || remaining > UINT32_MAX) {
            return;
        }
    }
    else {
        const size_t length = prefix.GetStringLength();
        const size_t prefixBytes = length / 2;
        if (length == 0 || (length & 1) != 0 || prefixBytes >= 8) {
            return;
        }
        remaining = 8 - prefixBytes;
    }

    const bool hasJob = (result.HasMember("job") && result["job"].IsObject()) || result.HasMember("job_id");
    setNativePrefix(prefix.GetString(), static_cast<uint32_t>(remaining),
                    notifyCurrentJob && !hasJob ? &result : nullptr);
}


int64_t xmrig::Client::submitNative(const JobResult &result)
{
    using namespace rapidjson;

    if (result.nativePayload.isNull() || result.nativePayload.isEmpty() || m_rpcId.isNull()) {
        return -1;
    }

    Document doc;
    if (doc.Parse(result.nativePayload.data(), result.nativePayload.size()).HasParseError() || !doc.IsObject() ||
        !doc.HasMember("method") || !doc["method"].IsString() || !doc.HasMember("params")) {
        return -1;
    }

    auto &allocator = doc.GetAllocator();
    if (doc.HasMember("id")) {
        doc["id"].SetInt64(m_sequence);
    }
    else {
        doc.AddMember("id", m_sequence, allocator);
    }

    Value &params = doc["params"];
    if (result.nativeArray) {
        if (!params.IsArray() || params.Empty() || m_pool.user().isNull()) {
            return -1;
        }

        params[0].SetString(m_pool.user().data(), static_cast<SizeType>(m_pool.user().size()), allocator);
    }
    else {
        if (!params.IsObject()) {
            return -1;
        }

        if (result.algorithm != Algorithm::PEARLHASH) {
            if (params.HasMember("id")) {
                params["id"].SetString(m_rpcId.data(), static_cast<SizeType>(m_rpcId.size()), allocator);
            }
            else {
                params.AddMember("id", Value(m_rpcId.data(), static_cast<SizeType>(m_rpcId.size()), allocator), allocator);
            }
        }
    }

    m_results[m_sequence] = SubmitResult(m_sequence, result.diff, result.actualDiff(), result.id, 0);
    m_results[m_sequence].assignedDiff = result.assignedDiff;
    m_results[m_sequence].minerIp = result.minerIp;

    // Use the submitted job's algorithm: a reused upstream may have logged in before Pearl was offered.
    return send(doc, result.algorithm == Algorithm::PEARLHASH);
}
