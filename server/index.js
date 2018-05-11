const express = require('express');
const bodyParser = require('body-parser');
const mongoose = require('mongoose');
const mqtt = require('async-mqtt');
const api = require('./api');
const config = require('./config');
const { Bin } = require('./models');

const mqttclient = mqtt.connect(`mqtt://${config.mqtt.broker_ip}:${config.mqtt.broker_port}`, {
    username: config.mqtt.auth.user,
    password: config.mqtt.auth.pwd,
    keepalive: config.mqtt.keepalive_interval
});
const app = express();

// Attaches two helper functions to send responses.
// They are needed to send responses (both 200s and errors)
// under the same JSON interface.
app.use((req, res, next) => {
    req.mqttclient = mqttclient;
    res.sendSuccess = function (obj) {
        res.status(200).send({
            status: 'ok',
            contents: obj
        });
    };
    res.sendError = function (error) {
        error = error || 'An internal error has occurred.';
        res.status(200).send({
            status: 'error',
            error: error
        });
    };
    next();
});

// Setup express
app.use(bodyParser.json());

// Mount all the routes
app.use('/api', api);
app.use((err, req, res, next) => {
    console.error(err);
    res.sendError();
});

// Connect to the MQTT broker
mqttclient.on('connect', async () => {

    // Connect to MongoDB
    try {
        mongoose.Promise = Promise;
        await mongoose.connect(`mongodb://${config.mongo.host}:${config.mongo.port}/${config.mongo.db}`);
    } catch (e) {
        console.error.bind(console, 'MongoDB connection error:');
        return;
    }

    // Publish to the MQTT topic the list of all the registered arduinos
    const bins = await Bin.find({ arduinoId: { $gt: 0 } }, 'arduinoId').exec();
    await Promise.all(bins.map(b => mqttclient.publish(config.mqtt.join_channel, b.arduinoId.toString())));

    // Starts the server
    app.listen(config.server.listen_port, () => console.log(`InternetOfTrash server started on port ${config.server.listen_port}.`));

});
mqttclient.on('error', err => {
    console.error('MQTT broker connection error:', err);
});