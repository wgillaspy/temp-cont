def changedFiles = []

pipeline {

    agent { label 'jenkins-bc-did' }

    options {
        disableConcurrentBuilds()
    }

    environment {
        IMAGE = "docker.pkg.github.com/wgillaspy/temp-cont/controller"
        IMAGE_TAG = "latest"
        CONTAINER_NAME = "controller" // The name of of the nodjs application that will read data coming of the controller board.
        APP_INO = "controller.ino"  // The name of the arduino file that will be compiled and written to the atmega.
        INO_CHANGED = "no"
        NODEJS_CHANGED = "no"
    }

    stages {
        stage("Login to github docker.") {
            steps {
                script {
                    withCredentials([usernamePassword(credentialsId: 'GITHUBUSER_TOKENPASS', usernameVariable: 'USER', passwordVariable: 'PASS')]) {
                        sh "docker login docker.pkg.github.com -u ${USER} -p ${PASS}"
                        env.GITDOCKERCREDENTAILS = "{\"username\":\"${USER}\",\"password\":\"${PASS}\",\"email\":\"${USER}@gmail.com\",\"serveraddress\":\"docker.pkg.github.com\"}".bytes.encodeBase64().toString()
                    }
                }
            }
        }

        // Not that I've ever reached the limit of writing to an atmega, but ideally you don't want to write to the
        // atmega if nothing changed.  This will also speed up deployments of the other code.
        // Since this project also contains circuit and 3d print files, we don't want to build the nodejs application
        // when those are changed, either.
        stage("See what files have changed") {

            steps {
                script {

                    for (changeLogSet in currentBuild.changeSets) {
                        for (entry in changeLogSet.getItems()) { // for each commit in the detected changes
                            for (file in entry.getAffectedFiles()) {
                                changedFiles.add(file.getPath()) // add changed file to list
                            }
                        }
                    }

                    changedFiles.unique()

                    for (fileName in changedFiles) {
                        println(fileName)
                        if (fileName.contains("arduino")) {
                            INO_CHANGED = "yes"
                        }
                        if (fileName.contains("nodejs")) {
                            NODEJS_CHANGED = "yes"
                        }
                    }
                }
            }
        }

        stage("Compile and deploy sketch") {
            when {
                expression {
                    INO_CHANGED == "yes"
                }
            }
            steps {
                script {

                    // Stop the container that is attached to the serial port.
                    sh "curl -X POST  -H 'Content-Type: application/json' http://${IOT_IP_AND_DOCKER_PORT}/containers/${CONTAINER_NAME}/stop"
                    sleep(10)

                    sh """
                        cd ./arduino/controller

                        cat ${APP_INO} | mo > ${APP_INO}.tmp
                        mv -f ${APP_INO}.tmp ${APP_INO}

                        tar -cvf controller.tar ./controller.ino ./Dockerfile

                        curl -X DELETE http://${IOT_IP_AND_DOCKER_PORT}/containers/avrdude-controller?v=latest
                        curl -X POST -H  "X-Registry-Auth: ${GITDOCKERCREDENTAILS}" -H 'Content-Type: application/x-tar' --data-binary '@controller.tar' http://${IOT_IP_AND_DOCKER_PORT}/build?t=avrdude-controller:latest
                        curl -X POST  -H 'Content-Type: application/json' --data-binary '@deploy-container.json' http://${IOT_IP_AND_DOCKER_PORT}/containers/create?name=avrdude-controller
                        curl -X POST  -H 'Content-Type: application/json' http://${IOT_IP_AND_DOCKER_PORT}/containers/avrdude-controller/start
                    """
                    // Replace this with a wait for that queries the docker rest API.
                    sleep(30)

                    waitUntil {
                        script {

                            CONTANER_STATUS = sh (
                                    script: "curl -X GET  -H 'Content-Type: application/json' http://${IOT_IP_AND_DOCKER_PORT}/containers/avrdude-controller/json",
                                    returnStdout: true
                            ).trim()

                            def containerStatusJson = readJSON text: "${CONTANER_STATUS}"
                            println containerStatusJson.State.Status
                            if (containerStatusJson.State.Status == "exited") {
                                return true
                            } else {
                                return false;
                            }
                        }
                    }


                    sh "curl -X POST  -H 'Content-Type: application/json' http://${IOT_IP_AND_DOCKER_PORT}/containers/${CONTAINER_NAME}/start"
                    sleep(10)
                }
            }
        }


        stage('Deploy the nodejs applications') {
            when {
                expression {
                    NODEJS_CHANGED == "yes"
                }
            }
            steps {
                script {

                    withCredentials([string(credentialsId: 'SPLUNK_URL', variable: 'SPLUNK_URL'),
                                     string(credentialsId: 'SPLUNK_TOKEN', variable: 'SPLUNK_TOKEN')]) {
                        sh """
                           cat deploy-container.json | mo > deploy-container.json.tmp
                           mv deploy-container.json.tmp > deploy-container.json
                         """
                    }

                    sh """
                        curl -X POST  -H 'Content-Type: application/json' http://${IOT_IP_AND_DOCKER_PORT}/containers/${CONTAINER_NAME}/stop
                        curl -X DELETE http://${IOT_IP_AND_DOCKER_PORT}/containers/${CONTAINER_NAME}?v=${IMAGE_TAG}

                        cd ./nodejs/src/main/controller

                        wget https://nodejs.org/dist/v10.16.2/node-v10.16.2-linux-armv7l.tar.xz

                        tar -cvf controller.tar package.json package-lock.json index.js node-v10.16.2-linux-armv7l.tar.xz ./Dockerfile

                        curl -X POST -H  "X-Registry-Auth: ${GITDOCKERCREDENTAILS}" -H 'Content-Type: application/x-tar' --data-binary '@controller.tar' http://${IOT_IP_AND_DOCKER_PORT}/build?t=controller:latest
                        curl -X POST  -H 'Content-Type: application/json' --data-binary '@deploy-container.json' http://${IOT_IP_AND_DOCKER_PORT}/containers/create?name=${CONTAINER_NAME}
                        curl -X POST  -H 'Content-Type: application/json' http://${IOT_IP_AND_DOCKER_PORT}/containers/${CONTAINER_NAME}/start
                  """
                }
            }
        }
    }
}
