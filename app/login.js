'use strict';

const nconf = require('nconf');
const rigs  = require('./rigs');

const BYPASS_WORKER_ID = nconf.get('bypass_worker_id');
const BYPASS_WALLET    = nconf.get('bypass_wallet');
const WALLET_ADDRESS   = nconf.get('wallet');
const PASSWORD         = nconf.get('password');
const USER_AGENT       = nconf.get('agent');


function login(socket, line) {
  if (line.charAt(0) !== '{') {
    throw new Error(line);
  }

  const packet = JSON.parse(line);
  if (packet.method !== 'login') {
    throw new Error('Invalid method');
  }

  rigs.add(socket, packet.params);
  packet.params.login = getLogin(packet.params.login);
  packet.params.agent = USER_AGENT;

  if (PASSWORD) {
    packet.params.pass = PASSWORD;
  }

  return JSON.stringify(packet) + '\n';
}


function getLogin(login) {
  login = login.split('.');
  if (!BYPASS_WALLET || login[0].length < 95) {
    login[0] = WALLET_ADDRESS;
  }

  if (!BYPASS_WORKER_ID || login.length < 2) {
    return login[0]
  }

  return login.join('.');
}


module.exports = login;
