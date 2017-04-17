'use strict';

const Promise  = require('bluebird');
const uuid     = require('uuid');
const net      = require('net');
const readline = require('readline');
const log      = require('./log');

const READY      = 0;
const WAIT_LOGIN = 1;
const WAIT_CLOSE = 2;


class Proxy {
  constructor(options, onLogin) {
    this.host    = options.host;
    this.port    = options.port;
    this.onLogin = onLogin;

    this.server = net.createServer(this.onConnection.bind(this));
  }


  listen(options) {
    return Promise.promisify(this.server.listen, { context: this.server })(options);
  }


  onConnection(socket) {
    socket.id = uuid.v4();

    _setupSocket(socket);
    socket.state = WAIT_LOGIN;

    const upstream = _setupSocket(new net.Socket());
    upstream.connect({ host: this.host, port: this.port });

    const i = readline.createInterface({ input: socket, terminal: false });
    i.on('line', line => {
      if (line === '') {
        return;
      }

      if (socket.state === WAIT_LOGIN) {
        try {
          upstream.write(this.onLogin(socket, line));
          socket.state = READY;
        }
        catch (e) {
          socket.state = WAIT_CLOSE;
          log.error(`[login] "${e.message}"`);
          socket.end();
        }

        return;
      }

      // console.log('<<<', line);
      upstream.write(line + '\n');
    });

    upstream.on('data', data => {
      // console.log('>>>', data.toString());
      socket.write(data)
    });

    socket.on('close', had_error => {
      upstream.end();
    });

    upstream.on('close', had_error => {
      socket.end();
    });
  }
}


function _setupSocket(socket) {
  socket.setNoDelay(true);
  socket.setKeepAlive(true, 120);

  socket.on('error', err => {});

  return socket;
}


module.exports = Proxy;
