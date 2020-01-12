// 10

const splunkjs = require('splunk-sdk');
const twilio = require('twilio');
const moment = require('moment');

const debounce = {}; // Place a hash of keys and times here.

const service = new splunkjs.Service({
    username: process.env.SPLUNK_USER,
    password: process.env.SPLUNK_PASSWORD,
    scheme: 'https',
    host: process.env.SPLUNK_API_HOST,
    port: process.env.SPLUNK_API_PORT,
    autologin: true
});

const searchQuery = "search index=iot";

const searchParams = {
    earliest_time: '-60s',
    latest_time: 'now',
    output_mode: 'json'
};

const allowSms = (label) => {

    if (debounce[label]) {

        const compareMoment = moment().subtract(30, "m");  // Only allow one message per type every thirty minutes.
        if (compareMoment.isAfter(debounce[label])) {
            debounce[label] = moment();
            return true;
        } else {
            return false;
        }

    } else {
        debounce[label] = moment();
        return true;
    }

};

const sendSMS = async (label, message) => {

    if (allowSms) {
        const client = new twilio(process.env.TWILIO_SID, process.env.TWILIO_AUTH_TOKEN);

        await client.messages.create({
            body: message,
            to:  process.env.TWILIO_TO,
            from:  process.env.TWILIO_FROM
        }).then((message) => {
            console.log(message);
        }).catch((exception) => {
            console.log(exception);
        });
    }
};

const alertIfTemperatureRangeExceeded = (avgTemp, tgtTemp) => {

    let difference = avgTemp - tgtTemp;
    if (difference < 0) {
        difference = difference * -1;
    }

    if (difference > 2) {
        sendSMS("TEMP", "The incubator temperature (" +  avgTemp + ") is out of range of the target temperature (" + tgtTemp + ") ");
    }

};

const alertIfArraySizeIsZero = (size) => {
    if (size == 0) {
        sendSMS("ARRAY", "The incubator temperature controller is not running.");
    }
};

const alertIfFanTicksTooLow = (fan) => {

    if (fan < 90) {
        sendSMS("FAN","The incubator temperature controller fan (" + fan + ") is not running correctly.");
    }
};

const searchSplunk = (service, searchQuery, searchParams) => {

    return new Promise((resolve, reject) => {

        service.oneshotSearch(searchQuery, searchParams, function (error, result) {
            if (error) {
                reject(error);
            } else {
                resolve(result.results);
            }
        });
    });
};


setInterval(function() {
    searchSplunk(service, searchQuery, searchParams).then(splunkResult => {

        alertIfArraySizeIsZero(splunkResult.length);
        for (const element of splunkResult) {
            const json = JSON.parse(element._raw);
            alertIfTemperatureRangeExceeded(json.avg, json.tgt);
            alertIfFanTicksTooLow(json.fan);
        }

    }).catch(exception => {
        console.log(exception);
    });
}, 30000);
