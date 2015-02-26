#/usr/bin/env python

import config
import json
import datetime
import time
import hashlib
import atexit
from sets import Set

try:
    from flask import Flask
    from flask import jsonify, render_template, request
except ImportError:
    print '[X] Please install Flask:'
    print '   $ pip install flask\n'
    exit()

try:
    import serial
except ImportError:
    print '[X] Please install Flask:'
    print '   $ pip install pySerial\n'
    exit()

try:
    rfm12pi = serial.Serial('/dev/ttyAMA0', baudrate=9600, timeout=3.0)
except OSError:
    rfm12pi = None
    print '[X] RFM12Pi not found. Start server anyway...'


app = Flask('simplehomeautomation')
active_switches = Set()


@app.route('/')
def main():
    return render_template(
        'index.html',
        config=config,
        active_switches=active_switches,
        logged_in=logged_in(request)
    )


@app.route('/login', methods=['POST'])
def login():
    if 'password' in request.form:
        if request.form['password'] == config.PASSWORD:
            return signed_response(jsonify({
                'status': True
            }))

    return jsonify({
        'status': False
    })


@app.route('/control', methods=['POST'])
def control():
    if not logged_in(request):
        return jsonify({
            'status': False
        })

    if all(x in request.form for x in ['system', 'device']):
        system = request.form['system']
        device = request.form['device']
        switch = ('switch-%s-%s' % (system, device))

        if 'state' in request.form:
            state = request.form['state']
        else:
            state = '0' if switch in active_switches else '1'

        # Send command if available
        if rfm12pi:
            rfm12pi.write('%s,%s,%se' % (system, device, state))

        # Remember status
        if state == '1':
            active_switches.add(switch)
        else:
            active_switches.discard(switch)

        return signed_response(jsonify({
            'status': True
        }))

    return signed_response(jsonify({
        'status': False
    }))


@app.route('/status')
def status():
    return jsonify({
        'switches': list(active_switches) if logged_in(request) else []
    })


def signed_response(response):
        # Add cookie
    expires = time.mktime((datetime.date.today() +
                           datetime.timedelta(days=7)).timetuple())
    response.set_cookie(
        config.COOKIE,
        value=str(current_secret()),
        expires=expires
    )
    return request


def logged_in(request):
    valid_cookie = (config.COOKIE in request.cookies and
                    request.cookies[config.COOKIE] == str(current_secret()))
    valid_secret = ('secret' in request.form and
                    request.form['secret'] == config.SECRET)
    return valid_cookie or valid_secret


def current_secret():
    return sha256(str(hash(app) * hash(config.SECRET)))


def sha256(string):
    return hashlib.sha224(string).hexdigest()


if __name__ == '__main__':
    app.run(host=config.HOST, port=config.PORT, debug=config.DEBUG)

    def close():
        print '[X] Shutting down server...'
        if rfm12pi:
            rfm12pi.close()
    atexit.register(close)
