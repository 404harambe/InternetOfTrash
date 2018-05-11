const fs = require('fs');
const ini = require('ini');

// Load the config file
const path = process.env['CONFIG_FILE'];
if (!path) {
    throw new Error('Missing config file path. Please, provide the path with the environment variable CONFIG_FILE.');
}

module.exports = ini.parse(fs.readFileSync(path, 'utf-8').toString());