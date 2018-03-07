pipeline {
    agent none
    stages {
        stage('Build') {
            parallel {
                stage('Ubuntu') {
                    agent { label 'Ubuntu' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo 1 | ./eosio_build.sh
                        '''
                    }
                }
                stage('MacOS') {
                    agent { label 'MacOS' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo 1 | ./eosio_build.sh 
                        ''' 
                    }
                }
                stage('Fedora') {
                    agent { label 'Fedora' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo 1 | ./eosio_build.sh 
                        ''' 
                    }
                }
            }
        }
        stage('Tests') {
            parallel {
                stage('Ubuntu') {
                    agent { label 'Ubuntu' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            cd build
                            printf "Waiting for testing to be available..."
                            while /usr/bin/pgrep -x ctest > /dev/null; do sleep 1; done
                            echo "OK!"
                            ctest --output-on-failure
                        '''
                    }
                }
                post {
                    failure {
                        archiveArtifacts 'build/tn_data_00/config.ini'
                        archiveArtifacts 'build/tn_data_00/stderr.txt'
                        archiveArtifacts 'build/test_walletd_output.log'
                    }
                }
                stage('MacOS') {
                    agent { label 'MacOS' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            cd build
                            printf "Waiting for testing to be available..."
                            while /usr/bin/pgrep -x ctest > /dev/null; do sleep 1; done
                            echo "OK!"
                            ctest --output-on-failure
                        '''
                    }
                }
                post {
                    failure {
                        archiveArtifacts 'build/tn_data_00/config.ini'
                        archiveArtifacts 'build/tn_data_00/stderr.txt'
                        archiveArtifacts 'build/test_walletd_output.log'
                    }
                }
                stage('Fedora') {
                    agent { label 'Fedora' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            cd build
                            printf "Waiting for testing to be available..."
                            while /usr/bin/pgrep -x ctest > /dev/null; do sleep 1; done
                            echo "OK!"
                            ctest --output-on-failure
                        '''
                    }
                }
                post {
                    failure {
                        archiveArtifacts 'build/tn_data_00/config.ini'
                        archiveArtifacts 'build/tn_data_00/stderr.txt'
                        archiveArtifacts 'build/test_walletd_output.log'
                    }
                }
            }
        }
    }
    post { 
        always { 
            node('Ubuntu') {
                cleanWs()
            }
            
            node('MacOS') {
                cleanWs()
            }

            node('Fedora') {
                cleanWs()
            }
        }
    }
}
