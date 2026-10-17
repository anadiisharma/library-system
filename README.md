# Library Management System - CI/CD on AWS (Jenkins, Docker, Kubernetes)

**Student:** [YOUR NAME], [PRN]  |  **Course:** Cloud Computing and DevOps (TYCSE), CCA 2

## 1. Project chosen
A web-based Library Management System. The backend is C++ (`server.cpp`, with a built-in HTTP server) and the
frontend is a single HTML page (`web/index.html`). Books are loaded from `books.csv`.
Members can search, issue (30-second demo loan) and return books with an automatic Rs. 5/second fine; a librarian
can view timelines and add books.

## 2. Repository layout
| Path | Purpose |
|---|---|
| `server.cpp`, `web/`, `books.csv` | Application source and data |
| `tests/smoke_test.sh` | Automated test used by the pipeline |
| `Dockerfile` | Multi-stage image build (compile, then small runtime image) |
| `Jenkinsfile` | Declarative pipeline |
| `k8s/deployment.yaml`, `k8s/service.yaml` | Kubernetes Deployment and Service (NodePort 30080) |

## 3. Pipeline stages
1. **Checkout** - Jenkins pulls the latest code from GitHub.
2. **Build and Test** - compiles `server.cpp` with warnings enabled, then runs the smoke test (starts the server, checks health, catalog loading, issue/return/fine rules and page serving).
3. **Docker Build** - builds `DOCKERHUB_USER/library-system:<build number>` from the Dockerfile.
4. **Push Image** - logs in with credentials stored in Jenkins (`dockerhub-creds`) and pushes the numbered tag and `latest` to Docker Hub.
5. **Deploy to Minikube** - sets the new image tag in the Deployment and runs `kubectl apply` for the Deployment and Service, then waits for the rollout.
6. **Verify** - lists pods, deployment and service, and calls the app's `/api/version` endpoint.

## 4. Jenkins on AWS
[Describe: EC2 t3.medium, Ubuntu 24.04, security group ports 22/8080/8081, installed Java 21, Jenkins, Docker, kubectl,
Minikube (run as the `jenkins` user), Jenkins plugins (suggested plugins incl. Git and Pipeline), credential
`dockerhub-creds`, pipeline job `library-pipeline` using "Pipeline script from SCM".]

## 5. Pushing the image
Jenkins logs in to Docker Hub with a stored access token and runs `docker push`. Images: https://hub.docker.com/r/[DOCKERHUB_USER]/library-system

## 6. Deploying and accessing the app
Minikube pulls the image from Docker Hub. The app is exposed by a NodePort Service (30080) and opened with
`kubectl port-forward --address 0.0.0.0 svc/library-service 8081:80`, then `http://<EC2-public-IP>:8081`.
Verification commands: `kubectl get pods`, `kubectl get deployment`, `kubectl get svc`.

## 7. Second run (code change)
[Describe the change, e.g. header text changed in `web/index.html` from "Central Library" to "Central Library - MIT WPU",
committed and pushed, Jenkins build #2 deployed image tag 2, and the footer changed from `version build-1` to `version build-2`.]

## 8. Run locally (optional)
    g++ -std=c++11 -O2 server.cpp -o library      (add -lws2_32 and use library.exe on Windows)
    ./library        then open http://localhost:8080

## Limitations
Book state lives inside the pod and resets when it restarts; single replica; no real authentication.
