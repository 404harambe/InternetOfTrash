/**
 * - `bin/:id/measurement`: used by the RPI to push an automatic measurement to the server.
 *   Message format:
 *   {
 *       timestamp: '...',
 *       binId: '...',
 *       value: 100
 *   }
 * 
 * - `bin/:id/update`: used by the server to request an update on a specific bin.
 *   Message format:
 *   {
 *       reqId: 0 // Opaque request id. Expected matching ID in the response.
 *   }
 * 
 * - `bin/:id/update/response`
 *   Message format:
 *   {
 *       reqId: 0,
 *       status: 'ok' | 'error',
 *       value: 100,                  // If status === 'ok'
 *       error: 'Lid not closed.'     // If status === 'error'
 *   }
 */

const mqtt = require('async-mqtt');
const config = require('./config');
const { Measurement } = require('./models');

const mqttclient = mqtt.connect(`mqtt://${config.mqtt.broker_ip}:${config.mqtt.broker_port}`, {
    username: config.mqtt.auth_user,
    password: config.mqtt.auth_psw,
    keepalive: parseInt(config.mqtt.keepalive_interval, 10)
});

// Cache of pending update requests
const pendingUpdateRequests = (function () {
    let i = 1;
    const map = {};
    return {
        create() {
            const id = i++;
            const promise = new Promise((resolve, reject) => {
                map[id] = { resolve, reject };
            });
            return { id, promise };
        },
        resolve(id, res) {
            if (map[id]) {
                map[id].resolve(res);
                delete map[id];
            }
        },
        reject(id, e) {
            if (map[id]) {
                map[id].reject(e);
                delete map[id];
            }
        }
    };
})();

// MQTT "routes"
const routes = [
    
    [ /^bin\/([\da-f]+)\/measurement$/i, async (payload, binId) => {
        try {
            const m = JSON.parse(payload);
            await Measurement.create(m);
            console.log('New measurement for bin ' + binId);
        } catch (e) {
            // Ignore invalid message
        }
    } ],

    [ /^bin\/([\da-f]+)\/update\/response$/i, (payload, binId) => {
        try {
            const res = JSON.parse(payload);
            if (typeof res.reqId !== 'undefined' && typeof res.status !== 'undefined') {
                console.log('Response for update request:', res);
                const reqId = parseInt(res.reqId, 10);
                if (res.status === 'ok') {
                    pendingUpdateRequests.resolve(res.reqId, res.value);
                } else {
                    pendingUpdateRequests.reject(res.reqId, res.error);
                }
            }
        } catch (e) {
            // Ignore the invalid message
        }
    } ]

];

// Message listener
mqttclient.on('message', (topic, payload) => {
    for (let i = 0; i < routes.length; i++) {
        const m = topic.match(routes[i][0]);
        if (m) {
            routes[i][1].call(null, payload, ...m.slice(1));
            break;
        }
    }
});

// Subscribe to the needed topics
Promise.all([
    mqttclient.subscribe('bin/+/measurement'),
    mqttclient.subscribe('bin/+/update/response')
])
.then(() => console.log('MQTT subscriptions completed.'))
.catch(console.error.bind(console, 'Error subscribing to MQTT topics.'));

// Export the native client and some helper functions
module.exports = {
    mqttclient,

    async requestUpdate(binId) {
        const { id, promise } = pendingUpdateRequests.create();
        await mqttclient.publish(`bin/${binId}/update`, JSON.stringify({ reqId: id }));
        return await promise;
    }
};