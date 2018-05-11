// A simple webpack loader to include the needed parts of the client config

const fs = require('fs');
const ini = require('ini');

module.exports = function (source) {
    this.cacheable && this.cacheable();
    const value = ini.parse(source.toString()).client;
    return 'module.exports = ' + JSON.stringify(value) + ';';
};