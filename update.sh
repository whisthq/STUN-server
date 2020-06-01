#!/bin/bash

# TODO: Automate updating the STUN-server code in Lightsail Instance(s)
curl -X POST --data-urlencode "payload={\"channel\": \"#general\", \"username\": \"fractal-bot\", \"text\": \"New production STUN server manually updated in AWS Lightsail\", \"icon_emoji\": \":ghost:\"}" https://hooks.slack.com/services/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
