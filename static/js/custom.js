// Initialize your app
var app = new Framework7();

// Export selectors engine
var $$ = Dom7;

// Add view
var mainView = app.addView('.view-main', {
    dynamicNavbar: true,
    domCache: true
});

// Handle push to refresh
var ptrContent = $$('.pull-to-refresh-content');
ptrContent.on('refresh', function (e) {
    setTimeout(function (e) {
        app.pullToRefreshDone();
    }, 2000);
});

// Click handler for login button
$$('#login-button').on('click', function (e) {
    app.closeModal('.login-screen');
});

// Click handler for custom button
$$('#custom-button').on('click', function (e) {
    console.log({
        system: $$('#custom-system').val(),
        device: $$('#custom-device').val(),
        state: ($$('#custom-state').prop('checked') ? "1" : "0")
    });
});

// Handle switch change
$$('.switch').on('change', function (e) {
    var target = $$('input', this);
    console.log({
        system: target.data('system'),
        device: target.data('device'),
        state: (target.prop('checked') ? "1" : "0")
    });
});
