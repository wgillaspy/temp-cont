// 9
const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline');
const moment = require('moment');
const axios = require("axios");


const lightControllerPort = new SerialPort('/dev/lightController', {
    baudRate: 9600
});

const temperatureControllerPort = new SerialPort('/dev/tempController', {
    baudRate: 9600
});

const newTemp = {"a": "tgt_tmp", "tgt": 85};
const newLight = {"a": "tgt_light", "tgt": 0};

const temperatureMap = {
    0: 83,
    1: 83,
    2: 83,
    3: 83,
    4: 83,
    5: 83,
    6: 83,
    7: 83,
    8: 83,
    9: 84,
    10: 85,
    11: 85,
    12: 85,
    13: 85,
    14: 85,
    15: 85,
    16: 85,
    17: 85,
    18: 85,
    19: 83,
    20: 83,
    21: 83,
    22: 83,
    23: 83
};

const lightMap = {
    0: 0,
    1: 0,
    2: 0,
    3: 0,
    4: 0,
    5: 60,
    6: 70,
    7: 120,
    8: 160,
    9: 220,
    10: 220,
    11: 255,
    12: 255,
    13: 255,
    14: 255,
    15: 255,
    16: 220,
    17: 160,
    18: 120,
    19: 60,
    20: 0,
    21: 0,
    22: 0,
    23: 0
};


const lightControllerParser = lightControllerPort.pipe(new Readline({delimiter: '\r\n'}));

const temperatureControllerParser = temperatureControllerPort.pipe(new Readline({delimiter: '\r\n'}));

lightControllerParser.on('data', function (data) {
    try {
        const json = JSON.parse(data);
        writeSplunkData(json);
        checkAndWriteData("Light", "lht", lightControllerPort, newLight, json, lightMap);
    } catch (parseException) {
        console.log("Json Parse Error:");
        console.log(parseException);
    }
});


temperatureControllerParser.on('data', function (data) {
    try {
        const json = JSON.parse(data);
        //console.log(json);
        writeSplunkData(json);
        checkAndWriteData("Temperature", "tgt", temperatureControllerPort, newTemp, json, temperatureMap);
    } catch (parseException) {
        console.log("Json Parse Error:");
        console.log(parseException);
    }
});

checkAndWriteData = (label, objectName, port, writejson, json, map) => {
    try {
        const hour = moment().format("H");

        if (json[objectName] != map[hour]) {

            console.log("[" + label + "] Hour: " + hour + ", " + json[objectName] + " != " + map[hour]);
            writejson.tgt = map[hour];
            port.write(JSON.stringify(writejson), function (error) {
                if (error) {
                    console.log(error);
                }
            });
        }

    } catch (err) {
        console.log(label + "Error:");
        console.log(err)
    }
};

writeSplunkData = (json) => {

    try {
        const eventObject = {"event": json};

        const axiosConfig = {
            headers: {
                "Authorization": "Splunk " + process.env.SPLUNK_TOKEN
            }
        };

        axios.post(process.env.SPLUNK_URL, eventObject, axiosConfig).then(response => {
        }).catch(error => {
        });

    } catch (splunkWriteDataError) {
        console.log("splunkWriteDataError:");
        console.log(splunkWriteDataError)
    }
};

