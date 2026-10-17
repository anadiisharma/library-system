# Library Management System - CI/CD on AWS (Jenkins, Docker, Kubernetes)

**Student:** Anadi Sharma, PRN:1262241420 | **Course:** Cloud Computing and DevOps (TYCSE), CCA 2
**Repository:** https://github.com/anadiisharma/library-system
**Docker Hub image:** https://hub.docker.com/r/anadiisharma/library-system

## 1. Project chosen
A web-based Library Management System built earlier as an academic project. The backend is written in C++
(`server.cpp`, with a small built-in HTTP server) and the frontend is a single HTML page (`web/index.html`).
Books are loaded from `books.csv`. Members can search, issue (30-second demo loan) and return books, and an
automatic fine of Rs. 5 per second is charged for late returns. A librarian can view timelines and add books.

## 2. Repository layout
| Path | Purpose |
|---|---|
| `server.cpp`, `web/`, `books.csv` | Application source and data |
| `tests/smoke_test.sh` | Automated test used by the pipeline |
| `Dockerfile` | Multi-stage image build (compile, then a small runtime image) |
| `Jenkinsfile` | Declarative pipeline |
| `k8s/deployment.yaml`, `k8s/service.yaml` | Kubernetes Deployment and Service (NodePort 30080) |

## 3. Pipeline stages
1. **Checkout** - Jenkins pulls the latest code from GitHub.
2. **Build and Test** - compiles `server.cpp` with warnings enabled, then runs the smoke test (starts the server, checks health, catalog loading, issue/return/fine rules and page serving).
3. **Docker Build** - builds `anadiisharma/library-system:<build number>` from the Dockerfile.
4. **Push Image** - logs in with credentials stored in Jenkins (`dockerhub-creds`) and pushes the numbered tag and `latest` to Docker Hub.
5. **Deploy to Minikube** - sets the new image tag in the Deployment and runs `kubectl apply` for the Deployment and Service, then waits for the rollout to finish.
6. **Verify** - lists pods, deployment and service, and calls the app's `/api/version` endpoint.

## 4. Jenkins on AWS
- AWS EC2 instance, Ubuntu Server 24.04 LTS, instance type [CHECK: c7i-flex.large or m7i-flex.large], 30 GiB gp3 disk, region ap-south-1 (Mumbai).
- Security group: port 22 (SSH), 8080 (Jenkins) and 8081 (website), with 8080 and 8081 restricted to my IP.
- Installed on the server: Java 21, Git, build tools (g++), Docker, Jenkins (LTS), kubectl and Minikube.
- Jenkins and Minikube run as the `jenkins` user (added to the `docker` group) so the pipeline can use Docker and the cluster.
- Jenkins plugins: the "suggested plugins" set, which includes Git and Pipeline.
- Credential `dockerhub-creds` (username with password; Docker Hub access token) stored in Jenkins global credentials.
- Pipeline job `library-pipeline`: "Pipeline script from SCM", Git repository above, branch `*/main`, script path `Jenkinsfile`.

## 5. Pushing the image
Jenkins logs in to Docker Hub using the stored access token (read from the credential, never written in the code) and runs
`docker push` for the tags `<build number>` and `latest`. Images: https://hub.docker.com/r/anadiisharma/library-system

## 6. Deploying and accessing the app
Minikube pulls the image from Docker Hub. The Deployment runs one pod with readiness and liveness probes on `/healthz`.
A NodePort Service (30080) exposes it. To open it from my laptop I ran
`kubectl port-forward --address 0.0.0.0 svc/library-service 8081:80` on the server and opened `http://<EC2-public-IP>:8081`.
Verification: `kubectl get pods`, `kubectl get deployment`, `kubectl get svc`, and the page footer shows the build number.

## 7. What I changed for the second run
I changed the page header in `web/index.html` from "Central Library" to "Central Library - MIT WPU", committed and pushed it
to GitHub, and clicked Build Now in Jenkins. Build #2 ran all six stages, pushed image tag `2`, and replaced the old pod with a
rolling update. The site then showed the new header and its footer changed from `version build-1` to `version build-2`.

## 8. Run locally (optional)
    g++ -std=c++11 -O2 server.cpp -o library      (on Windows add -lws2_32 and name it library.exe)
    ./library        then open http://localhost:8080

## Limitations
Book state lives inside the pod and resets when it restarts; there is a single replica; and there is no real authentication.
