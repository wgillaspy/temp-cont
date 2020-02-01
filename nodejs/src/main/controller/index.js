// 2
const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline');
const moment = require('moment');
const axios = require("axios");


const port = new SerialPort('/dev/tempController', {
    baudRate: 9600
});

const newTemp = {"a": "tgt_tmp", "tgt": 85};

const temperatureMap = {
    0: 83,
    1: 82,
    2: 81,
    3: 80,
    4: 80,
    5: 80,
    6: 81,
    7: 82,
    8: 83,
    9: 84,
    10: 85,
    11: 86,
    12: 87,
    13: 89,
    14: 90,
    15: 88,
    16: 87,
    17: 86,
    18: 85,
    19: 83,
    20: 83,
    21: 83,
    22: 83,
    23: 83
};


const parser = port.pipe(new Readline({delimiter: '\r\n'}));

parser.on('data', function (data) {

    try {
        const hour = moment().format("H");
        const json = JSON.parse(data);
        //console.log(json);

        const eventObject = {"event" : json};

        const axiosConfig = {
            headers: {
                "Authorization" : "Splunk " + process.env.SPLUNK_TOKEN
            }
        };

        axios.post(process.env.SPLUNK_URL, eventObject, axiosConfig).then(response => {

        }).catch(error => {

        });

        if (json.tgt != temperatureMap[hour]) {
            console.log("Hour: " + hour + ", " + json.tgt + " != " + temperatureMap[hour]);


            newTemp.tgt = temperatureMap[hour];
            port.write(JSON.stringify(newTemp), function (error) {
                if (error) {
                    console.log(error);
                }
            });
        }

    } catch (err) {
        console.log("Error:");
        console.log(err)
    }

});


