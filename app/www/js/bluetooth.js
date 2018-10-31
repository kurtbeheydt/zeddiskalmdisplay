var bluetooth = {
    serviceUuids: { // Nordic's UART service
        serviceUUID: '6E400001-B5A3-F393-E0A9-E50E24DCCA9E',
        txCharacteristic: '6E400002-B5A3-F393-E0A9-E50E24DCCA9E', // transmit is from the phone's perspective
        rxCharacteristic: '6E400003-B5A3-F393-E0A9-E50E24DCCA9E' // receive is from the phone's perspective    
    },
    writeWithoutResponse: true,
    connectedDevice: {},
    lastConnectedDeviceId: false,
    messages: [],
    initialize: function () {
        debug.log('Initialising bluetooth ...');
        bluetooth.refreshDeviceList();
        debug.log('Bluetooth Initialised', 'success');
    },
    refreshDeviceList: function () {
        var onlyUART = true;
        $('#ble-found-devices').empty();
        var characteristics = (onlyUART) ? [bluetooth.serviceUuids.serviceUUID] : [];
        ble.scan(characteristics, 5, bluetooth.onDiscoverDevice, app.onError);
    },
    onDiscoverDevice: function (device) {
        var previousConnectedDevice = storage.getItem('connectedDevice');

        //if (device.name.toLowerCase().replace(/[\W_]+/g, "").indexOf('cme') > -1) {
        var html = '<ons-list-item modifier="chevron" data-device-id="' + device.id + '" data-device-name="' + device.name + '" tappable>' +
            '<span class="list-item__title">' + device.name + '</span>' +
            '<span class="list-item__subtitle">' + device.id + '</span>' +
            '</ons-list-item>';

        $('#ble-found-devices').append(html);
        $('#ble-results').show();
        //}

        if (previousConnectedDevice) {
            if (device.id == previousConnectedDevice.id) {
                debug.log('discovered previous device ' + previousConnectedDevice.name, 'success');
                bluetooth.connectDevice(previousConnectedDevice.id, previousConnectedDevice.name);
            }
        }

    },
    connectDevice: function (deviceId, deviceName) {
        debug.log('connecting to ' + deviceId);

        var onConnect = function (peripheral) {
            bluetooth.connectedDevice = {
                id: deviceId,
                name: deviceName
            };

            // used to send disconnected messages 
            bluetooth.lastConnectedDedviceId = deviceId;

            storage.setItem('connectedDevice', bluetooth.connectedDevice);

            // subscribe for incoming data
            ble.startNotification(deviceId,
                bluetooth.serviceUuids.serviceUUID,
                bluetooth.serviceUuids.rxCharacteristic,
                bluetooth.onData,
                bluetooth.onError);

            debug.log('Connected to ' + deviceId, 'success');

            bluetooth.toggleConnectionButtons();
        };

        ble.connect(deviceId, onConnect, bluetooth.onError);
    },
    onDisconnectDevice: function () {
        storage.removeItem('connectedDevice');
        debug.log('Disconnected from ' + bluetooth.lastConnectedDeviceId, 'success');
        bluetooth.connectedDevice = {};
        bluetooth.toggleConnectionButtons()();
    },
    disconnectDevice: function (event) {
        debug.log('Disconnecting from ' + bluetooth.connectedDevice.id);

        try {
            ble.disconnect(bluetooth.connectedDevice.id, bluetooth.onDisconnectDevice, bluetooth.onError);
            bluetooth.toggleConnectionButtons();
        } catch (error) {
            debug.log('Disconnecting failed', 'error');
            console.log(error);
        }
    },
    onData: function (data) {

        bluetooth.messages.push({
            data: bytesToString(data),
            timestamp: moment().format()
        })

        console.log(bytesToString(data));
    },
    sendData: function (message) {
        var success = function () {
            console.log("success");
        };

        var failure = function () {
            console.log("Failed writing data");
        };

        var messageLines = message.split('|');

        for (var i = 0; i < messageLines.length; i++) {
            var messageLine = messageLines[i].toUpperCase();
            console.log(messageLine);
            var data = stringToBytes(i + messageLine);
            ble.write(bluetooth.connectedDevice.id, bluetooth.serviceUuids.serviceUUID, bluetooth.serviceUuids.txCharacteristic, data.buffer, success, failure);
        }
    },
    onError: function (reason) {
        debug.log("BLE error: " + JSON.stringify(reason), 'error');
        ble.isConnected(bluetooth.connectedDevice.id, function () {
            debug.log('error, but still connected');
        }, function () {
            bluetooth.connectedDevice = {};
            debug.log('error and disconnected from ' + bluetooth.lastConnectedDeviceId, 'success');
            bluetooth.toggleConnectionButtons()();
        });
    },
    toggleConnectionButtons: function () {
        var connected = (bluetooth.connectedDevice.id !== undefined);
        console.log('current ble connection status: ' + ((connected) ? 'connected' : 'not connected'));

        if (connected) {
            $('.ble-not-connected').hide();
            $('.ble-connected').show();
        } else {
            $('#ble-connected-device').html('no device connected');
            $('.ble-not-connected').show();
            $('.ble-connected').hide();
        }
    }
};

/*
helpers
*/
// ASCII only
function bytesToString(buffer) {
    return String.fromCharCode.apply(null, new Uint8Array(buffer));
}

// ASCII only
function stringToBytes(string) {
    var array = new Uint8Array(string.length);
    for (var i = 0, l = string.length; i < l; i++) {
        array[i] = string.charCodeAt(i);
    }
    return array;
}