'use strict';

const winston = require('winston');


const logger = new (winston.Logger)({
  transports: [
    new winston.transports.File({ filename: __dirname + '/../log/app.log', json: false })
  ],
  exitOnError: true
});


/* istanbul ignore next */
if (process.env.NODE_ENV !== 'production') {
  logger.add(winston.transports.Console, { colorize: true, timestamp: true, level: 'debug' });
}


module.exports = logger;
