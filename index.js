'use strict';

const config = require('./app/config');
const nconf  = require('nconf');
const path   = require('path');
const fs     = require('fs');
const Proxy  = require('./app/Proxy');
const log    = require('./app/log');
const login  = require('./app/login');
const rigs   = require('./app/rigs');


nconf.get('proxy').forEach(listen => {
  const proxy = new Proxy(nconf.get('upstream'), login);
  proxy.listen(listen)
    .then(() => log.info('[app] listen:', listen));
});


setInterval(() => {
  fs.writeFile(path.join(__dirname, `log/report.json`), JSON.stringify(rigs.report(), null, 2), err => {})
}, 60000);
