# General
TITLE = "Home Control"

# Security config
PASSWORD = "1234"
SECRET = "81dc9bdb52d04dc20036dbd8313ed055"

# Server config
HOST = "0.0.0.0"
PORT = 8000
DEBUG = False

# Switch list
SWITCHES = [
    {
        "name": "Room",
        "system": 8,
        "device": 1,
        "icon": "ion-ios-lightbulb",
    },
    {
        "name": "Desk",
        "system": 16,
        "device": 2,
        "icon": "ion-ios-lightbulb",
    },
    {
        "name": "Bed",
        "system": 8,
        "device": 3,
        "icon": "ion-ios-lightbulb",
    },
    {
        "name": "Fan",
        "system": 16,
        "device": 1,
        "icon": "ion-nuclear",
    },
]

# Misc
COOKIE = "simplehomeautomation"
ENABLE_CUSTOM_CODES = True