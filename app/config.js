'use strict';

const Promise = require('bluebird');
const nconf   = require('nconf');
const path    = require('path');


let suffix = '-dev';

if (process.env.NODE_ENV === 'production') {
  suffix = '';
}
else if (process.env.NODE_ENV === 'test') {
  suffix = '-test';
}


nconf.file(path.join(__dirname, `/../config/app${suffix}.json`));
nconf.file('default', path.join(__dirname, '/../config/default.json'));


module.exports.save = function() {
  return Promise.promisify(nconf.save, {context: nconf})();
};
