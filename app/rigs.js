'use strict';

const log = require('./log');


const STORE = new Map();


/**
 * Add new rig.
 *
 * @param {Object} socket
 * @param {Object} params
 */
function add(socket, params) {
  STORE.set(socket.id, { id: socket.id, ip: socket.remoteAddress, login: params.login, ua: params.agent, datetime: +new Date() });

  socket.on('close', had_error => {
    remove(socket.id, had_error);
  });

  log.info(socket.remoteAddress, `login: "${params.login}", ua: "${params.agent}", count: ${STORE.size}`);
}


function remove(socket_id, had_error) {
  const info = STORE.get(socket_id);
  if (!info) {
    return;
  }

  STORE.delete(socket_id);
  log.info(info.ip, `close: "${info.login}", had_error: ${had_error}, count: ${STORE.size}`);
}


function report() {
  return {
    rigsCount: STORE.size,
    rigs:      Array.from(STORE.values())
  };
}


module.exports.add    = add;
module.exports.remove = remove;
module.exports.report = report;
