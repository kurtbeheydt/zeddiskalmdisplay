var app = {
    user: {},
    debug: false,
    initialize: function () {
        console.log('app initialize');
        document.addEventListener('deviceready', this.onDeviceReady.bind(this), false);
        /*console.log(window.location.hostname);
        if (window.location.hostname == 'bicyclelight.localhost') {
            app.debug = true;
            ons.ready(function () {
                app.onDeviceReady();
            });
        }*/
    },

    onDeviceReady: function () {
        debug.log('device ready', 'success');
        app.bindEvents();
        bluetooth.initialize();
    },

    bindEvents: function () {
        document.addEventListener("pause", app.onDevicePause, false);
        document.addEventListener("resume", app.onDeviceResume, false);
        document.addEventListener("menubutton", app.onMenuKeyDown, false);
    },

    sendMessage: function(message) {
        console.log('sendMessage' + message);
    },

    onDevicePause: function () {
        debug.log('in pause');
    },
    onDeviceResume: function () {
        debug.log('out of pause');
        bluetooth.refreshDeviceList();
    },
    onMenuKeyDown: function () {
        debug.log('menubuttonpressed');
    },
    onError: function (error) {
        debug.log(JSON.stringify(error), 'error');
    }
};

app.initialize();