// Initialize your app
var app = new Framework7();

// Export selectors engine
var $$ = Dom7;

// Add view
var mainView = app.addView('.view-main', {
    dynamicNavbar: true,
    domCache: true
});

// Handle login attempts
$$('form.ajax-submit').on('submitted', function (e) {
    var data = JSON.parse(e.detail.xhr.responseText);
    if (data.status == true) {
        app.closeModal('.login-screen')
    } else {
        app.addNotification({
            title: 'Password incorrect',
            message: 'Please try again...',
            closeIcon: false,
            hold: 1750
        });
    }
});

// Handle push to refresh
var ptrContent = $$('.pull-to-refresh-content');
ptrContent.on('refresh', function (e) {
    $$.getJSON('status', function(data){
            app.pullToRefreshDone();
    });
});

// Click handler for login button
$$('#login-button').on('click', function (e) {
    $$('#login-form').trigger('submit');
});

// Click handler for custom button
$$('#custom-button').on('click', function (e) {
    $$.post('control', {
        system: $$('#custom-system').val(),
        device: $$('#custom-device').val(),
        state: ($$('#custom-state').prop('checked') ? "1" : "0")
    });
});

// Handle switch change
$$('.switch').on('change', function (e) {
    var target = $$('input', this);
    $$.post('control', {
        system: target.data('system'),
        device: target.data('device'),
        state: (target.prop('checked') ? "1" : "0")
    });
});

// UI refresh
function refresh() {
    $$.getJSON('status', function(data){
        $$('.switch input').prop('checked', false);
        for (var i = data['switches'].length - 1; i >= 0; i--) {
            var remote_switch = data['switches'][i];
            $$('#' + remote_switch).prop('checked', true);
        };
    });
}

// Refresh UI periodically
setInterval(function(){
    refresh();
}, 2500);