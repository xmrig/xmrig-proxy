/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef XMRIG_NO_GOOGLE_BREAKPAD
#   include "client/linux/handler/exception_handler.h"

static bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor, void *context, bool succeeded) {
    printf("Dump path: %s\n", descriptor.path());
    return succeeded;
}
#endif

#include "ProxyApp.h"
#include "base/kernel/Entry.h"
#include "base/kernel/Process.h"
#include "core/config/usage.h"


int main(int argc, char **argv) {
#   ifndef XMRIG_NO_GOOGLE_BREAKPAD
    google_breakpad::MinidumpDescriptor descriptor("/tmp");
    google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback, NULL, true, -1);
#   endif

    using namespace xmrig;

    Process process(argc, argv);

    {
        int rc = 0;
        auto entry = std::make_unique<Entry>(usage);

        if (entry->exec(rc)) {
            return rc;
        }
    }

    ProxyApp app;

    return app.exec();
}
