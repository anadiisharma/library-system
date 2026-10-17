pipeline {
    agent any

    environment {
        // Jenkins credential (kind: Username with password) holding your Docker Hub login + access token
        DOCKERHUB = credentials('dockerhub-creds')
        IMAGE     = "${DOCKERHUB_USR}/library-system"
        TAG       = "${BUILD_NUMBER}"
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm                      // pulls the latest code from GitHub
            }
        }

        stage('Build and Test') {
            steps {
                sh 'g++ -std=c++11 -O2 -Wall server.cpp -o library'
                sh 'sh tests/smoke_test.sh ./library'
            }
        }

        stage('Docker Build') {
            steps {
                sh 'docker build --build-arg APP_VERSION=build-${TAG} -t ${IMAGE}:${TAG} -t ${IMAGE}:latest .'
            }
        }

        stage('Push Image') {
            steps {
                sh 'echo "$DOCKERHUB_PSW" | docker login -u "$DOCKERHUB_USR" --password-stdin'
                sh 'docker push ${IMAGE}:${TAG}'
                sh 'docker push ${IMAGE}:latest'
            }
        }

        stage('Deploy to Minikube') {
            steps {
                sh 'sed -i "s|DOCKERHUB_USER/library-system:latest|${IMAGE}:${TAG}|" k8s/deployment.yaml'
                sh 'kubectl apply -f k8s/deployment.yaml -f k8s/service.yaml'
                sh 'kubectl rollout status deployment/library-app --timeout=120s'
            }
        }

        stage('Verify') {
            steps {
                sh 'kubectl get pods -o wide'
                sh 'kubectl get deployment library-app'
                sh 'kubectl get svc library-service'
                sh 'curl -fs http://$(minikube ip):30080/api/version'
            }
        }
    }

    post {
        always { sh 'docker logout || true' }
        success { echo "Deployed ${IMAGE}:${TAG}. Open the app and check the footer shows build-${TAG}." }
    }
}
