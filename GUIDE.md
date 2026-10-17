# CCA 2 - Complete Step-by-Step Guide
Library Management System -> GitHub -> Jenkins (on AWS) -> Docker Hub -> Minikube

Read each part fully, do the steps in order, and do not skip the "Check" lines.
Never paste your passwords, access tokens or .pem key anywhere public (not in GitHub, not in chats).

Big picture of what you are building:

    your laptop --git push--> GitHub --> Jenkins (on AWS server) --> builds + tests code
                                              |--> builds Docker image --> pushes to Docker Hub
                                              |--> deploys to Minikube (Kubernetes on the same AWS server)
                                                         --> you open the website and show it

Time needed: about 2 to 3 hours the first time.

=====================================================================
PART 1 - Put the project on GitHub
=====================================================================

1.1  Make a GitHub account (skip if you have one): https://github.com -> Sign up.

1.2  Install Git on your Windows laptop:
     - Go to https://git-scm.com/download/win , the download starts automatically.
     - Run the installer and keep clicking Next (defaults are fine), then Finish.
     - Close and re-open any Command Prompt windows afterwards.

1.3  Tell Git who you are (one time). Open Command Prompt and type these two lines, replacing the text in quotes:
         git config --global user.name "Anadi Sharma"
         git config --global user.email "your-github-email@example.com"

1.4  Create the empty repository on GitHub:
     - Click the + at the top right of github.com -> New repository.
     - Repository name:  library-system
     - Choose  Public  (this lets Jenkins download it without extra passwords).
     - Do NOT tick "Add a README", .gitignore or license (we already have these).
     - Click Create repository.
     - Keep the page open. Copy the HTTPS address shown, like:
         https://github.com/YOURUSERNAME/library-system.git

1.5  Get the project folder ready:
     - Extract the new zip (right-click -> Extract All).
     - Open the folder that directly contains  Dockerfile, Jenkinsfile, server.cpp, books.csv, k8s, web, tests.
     - Click the address bar at the top of File Explorer, type  cmd  and press Enter.
       A Command Prompt opens inside this folder.
     - Check:  type  dir  -> you must see Dockerfile, Jenkinsfile, server.cpp, k8s.

1.6  Push the code. Type these one at a time (replace YOURUSERNAME):
         git init -b main
         git add .
         git commit -m "Library management system with CI/CD files"
         git remote add origin https://github.com/YOURUSERNAME/library-system.git
         git push -u origin main
     A window pops up asking you to sign in to GitHub in the browser: sign in and click Authorize.
     (If it asks for a password in the terminal instead, use a Personal Access Token:
      GitHub -> profile picture -> Settings -> Developer settings -> Personal access tokens -> Tokens (classic)
      -> Generate new token -> tick "repo" -> copy it and paste it as the password.)

     Check: refresh your GitHub repo page. You must see Dockerfile, Jenkinsfile, k8s, web, server.cpp.
     Take SCREENSHOT 1: the GitHub repository page.

=====================================================================
PART 2 - Docker Hub account and access token
=====================================================================

2.1  Sign up at https://hub.docker.com . Remember your USERNAME (example: anadi123).
2.2  Create the repository: Repositories -> Create repository -> Name: library-system -> Visibility: Public -> Create.
2.3  Create an access token (Jenkins will use this instead of your password):
     - Click your avatar (top right) -> Account settings -> Personal access tokens (under Security).
     - Generate new token -> Description: jenkins -> Access permissions: Read, Write, Delete -> Generate.
     - Copy the token NOW and save it in Notepad. It is shown only once.

=====================================================================
PART 3 - Create the AWS server (EC2)
=====================================================================

3.1  Sign in at https://aws.amazon.com/console . (AWS Academy students: open your Learner Lab, click Start Lab,
     wait for the green dot, then click AWS.)
3.2  Top right: choose a region near you, e.g. Asia Pacific (Mumbai) ap-south-1. Use the same region always.
3.3  Search "EC2" in the top search bar -> open it -> click  Launch instance  and set:
       Name:               jenkins-server
       Application image:  Ubuntu -> "Ubuntu Server 24.04 LTS"
       Instance type:      t3.medium   (2 CPU, 4 GB RAM. Minikube does NOT run properly on t2.micro/t3.micro.)
       Key pair:           Create new key pair -> name: jenkins-key -> RSA -> .pem -> Create (a file downloads; keep it safe)
       Network settings:   click Edit, then
                             - tick "Create security group"
                             - Rule 1 (already there): SSH, port 22, Source: Anywhere (needed for the browser connect button)
                             - Add security group rule: Custom TCP, port 8080, Source: My IP    (Jenkins)
                             - Add security group rule: Custom TCP, port 8081, Source: My IP    (our website)
       Storage:            30 GiB gp3
     Click Launch instance.
     (Cost: a t3.medium costs a few rupees per hour. STOP the instance when you are not working: see Part 10.)
3.4  Click the instance id -> wait until "Instance state: Running" and "Status check: 2/2 passed".
     Copy the "Public IPv4 address" (example 13.233.10.20). Write it in Notepad.
     NOTE: this IP changes every time you stop/start the instance. Look it up again each time.
3.5  Connect: select the instance -> click  Connect  -> tab "EC2 Instance Connect" -> user name ubuntu -> Connect.
     A black terminal opens in your browser. Everything in Parts 4 to 7 that says "on the server" is typed here.
     Check: you see a prompt like  ubuntu@ip-172-31-x-x:~$
     Take SCREENSHOT 2: EC2 console showing the running instance (with its security group rules if you can).

=====================================================================
PART 4 - Install the tools on the server (type these in the server terminal)
=====================================================================
Copy a block, paste (right-click or Ctrl+Shift+V), press Enter, wait until the prompt returns.

4.1  Update and install basics:
         sudo apt update
         sudo apt install -y git curl build-essential fontconfig openjdk-21-jre
         java -version
     Check: java -version prints "openjdk version 21...".

4.2  Install Docker:
         sudo apt install -y docker.io
         sudo systemctl enable --now docker
         docker --version

4.3  Install Jenkins (official steps):
         sudo mkdir -p /etc/apt/keyrings
         sudo wget -O /etc/apt/keyrings/jenkins-keyring.asc https://pkg.jenkins.io/debian-stable/jenkins.io-2026.key
         echo "deb [signed-by=/etc/apt/keyrings/jenkins-keyring.asc] https://pkg.jenkins.io/debian-stable binary/" | sudo tee /etc/apt/sources.list.d/jenkins.list > /dev/null
         sudo apt update
         sudo apt install -y jenkins
         sudo systemctl enable --now jenkins
         sudo systemctl status jenkins --no-pager
     Check: status shows "active (running)". (Press q if the screen waits.)
     If the key URL ever fails, open https://www.jenkins.io/doc/book/installing/linux/ and copy the current
     "Debian/Ubuntu - Long Term Support release" commands from there.

4.4  Give Jenkins permission to use Docker:
         sudo usermod -aG docker jenkins
         sudo usermod -aG docker ubuntu
         sudo systemctl restart jenkins

4.5  Install kubectl (the Kubernetes command tool):
         sudo snap install kubectl --classic
         kubectl version --client

4.6  Install Minikube:
         curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube-linux-amd64
         sudo install minikube-linux-amd64 /usr/local/bin/minikube
         minikube version

=====================================================================
PART 5 - Start Minikube (as the jenkins user, so the pipeline can use it)
=====================================================================
Jenkins runs commands as a user called "jenkins", so the Kubernetes cluster must belong to that user.

5.1  Switch to that user:
         sudo su - jenkins -s /bin/bash
     Your prompt changes to  jenkins@ip-...$
5.2  Start the cluster (takes 2 to 5 minutes the first time):
         minikube start --driver=docker --cpus=2 --memory=2800mb
5.3  Check:
         kubectl get nodes
     You must see one node named "minikube" with STATUS Ready.
5.4  Leave this user for now:
         exit
     (Whenever you stop and start the AWS server later, run 5.1 and 5.2 again: minikube does not auto-start.)

=====================================================================
PART 6 - Set up Jenkins in the browser
=====================================================================

6.1  On your laptop open:  http://YOUR-SERVER-IP:8080
     If it will not load: your "My IP" in the security group changed (new wifi). EC2 -> Security Groups ->
     your group -> Edit inbound rules -> set 8080 and 8081 Source to "My IP" again -> Save.
6.2  Unlock Jenkins. On the server type:
         sudo cat /var/lib/jenkins/secrets/initialAdminPassword
     Copy the long text and paste it into the browser page -> Continue.
6.3  Click "Install suggested plugins" and wait. (This installs Git and Pipeline, which we need.)
6.4  Create First Admin User: fill username, password, name, email -> Save and Continue -> Save and Finish -> Start using Jenkins.
6.5  Add your Docker Hub login to Jenkins securely:
       Manage Jenkins -> Credentials -> (global) under "Stores scoped to Jenkins" click "System" -> "Global credentials (unrestricted)"
       -> Add Credentials
         Kind:      Username with password
         Username:  your Docker Hub username
         Password:  the Docker Hub access token from step 2.3 (NOT your account password)
         ID:        dockerhub-creds      <- must be exactly this, the Jenkinsfile uses this name
       -> Create.
     Take SCREENSHOT 3: the credentials list showing dockerhub-creds (the password is hidden).
6.6  Create the pipeline job:
       Dashboard -> New Item -> name: library-pipeline -> choose "Pipeline" -> OK.
       Scroll to the Pipeline section:
         Definition:        Pipeline script from SCM
         SCM:               Git
         Repository URL:    https://github.com/YOURUSERNAME/library-system.git
         Branch Specifier:  */main
         Script Path:       Jenkinsfile
       -> Save.
     Take SCREENSHOT 4: the job configuration page (Pipeline section).

=====================================================================
PART 7 - Run the pipeline (first run)
=====================================================================

7.1  In the job page click  Build Now. A row "#1" appears under Build History. Click it, then "Console Output".
7.2  Wait 3 to 6 minutes (the first Docker build downloads images). The stage view shows 6 stages:
     Checkout, Build and Test, Docker Build, Push Image, Deploy to Minikube, Verify.
     All green and the last line says "Finished: SUCCESS".
     Take SCREENSHOT 5: the Stage View with all stages green.
     Take SCREENSHOT 6: the Console Output showing the Verify stage (pods, deployment, service and the version).
7.3  Docker Hub check: open hub.docker.com -> your library-system repo -> Tags shows "1" and "latest".
     Take SCREENSHOT 7: Docker Hub tags.
7.4  Kubernetes check, on the server:
         sudo su - jenkins -s /bin/bash
         kubectl get pods
         kubectl get deployment
         kubectl get svc
     You should see the pod "library-app-..." Running 1/1, deployment READY 1/1, service library-service with 80:30080/TCP.
     Take SCREENSHOT 8: this terminal.
7.5  Open the website. Still as the jenkins user on the server, type:
         kubectl port-forward --address 0.0.0.0 svc/library-service 8081:80
     It prints "Forwarding from 0.0.0.0:8081" and stays running (do not close it).
     On your laptop open:  http://YOUR-SERVER-IP:8081
     You see the library website. Scroll to the bottom: the footer says "version build-1".
     Take SCREENSHOT 9: the website in the browser (footer visible, address bar with the IP visible).
     Stop the port-forward with Ctrl+C when finished.

=====================================================================
PART 8 - Change the code and run again (required by the assignment)
=====================================================================

8.1  On your laptop open web\index.html in Notepad (or VS Code).
     Find the line:   <h1>📚 Central Library</h1>
     Change it to:    <h1>📚 Central Library - MIT WPU</h1>
     Save.
8.2  In the Command Prompt in your project folder:
         git add .
         git commit -m "Change header text"
         git push
8.3  In Jenkins click  Build Now  again. Build #2 runs and goes green.
     (The pipeline builds a new image tagged 2 and replaces the old pod: a "rolling update".)
8.4  On the server (jenkins user) start the port-forward again (it breaks when the old pod is replaced):
         kubectl port-forward --address 0.0.0.0 svc/library-service 8081:80
     Refresh http://YOUR-SERVER-IP:8081 (press Ctrl+F5). The header shows "- MIT WPU" and the footer says "version build-2".
     Take SCREENSHOT 10: build #2 stage view all green.
     Take SCREENSHOT 11: the website showing the new header and build-2.
     Also useful: SCREENSHOT 12: Docker Hub tags now showing 1, 2 and latest.

=====================================================================
PART 9 - What to submit
=====================================================================
- GitHub repository URL  (https://github.com/YOURUSERNAME/library-system)
- Jenkinsfile, Dockerfile, k8s/deployment.yaml, k8s/service.yaml  (all are already in the repo)
- The screenshots above
- README: open README.md in the repo, replace every [PLACEHOLDER], fill in "What I changed for the second run"
  with step 8.1, then commit and push. This is your report.

=====================================================================
PART 10 - Stopping, restarting, and saving money
=====================================================================
Stop when done for the day: EC2 console -> select the instance -> Instance state -> Stop instance. (You do not pay for compute while stopped.)
Start again: Instance state -> Start instance. Then:
  1. Look up the NEW public IP.
  2. Connect, and run:  sudo su - jenkins -s /bin/bash   then   minikube start
  3. Jenkins comes back by itself at http://NEW-IP:8080
  4. If the pod is missing: Build Now again.
After the assignment is graded: Instance state -> Terminate instance (deletes everything), and delete unused keys.

=====================================================================
TROUBLESHOOTING (what the error means -> what to do)
=====================================================================
- "git is not recognized"           -> Git not installed or the window was opened before installing. Close and reopen cmd.
- "remote origin already exists"    -> run  git remote remove origin  then repeat the add and push lines.
- Jenkins page does not open        -> security group port 8080 / wrong IP (IP changes on restart) / "My IP" changed.
- Stage Checkout fails              -> repo URL typo or the repo is Private. Make it Public.
- "g++: command not found"          -> step 4.1 not done. Run:  sudo apt install -y build-essential
- Docker stage: "permission denied ... docker.sock" -> step 4.4 not done. Run it, then:  sudo systemctl restart jenkins
- Push stage: "denied" / "unauthorized" -> wrong credential. Re-create dockerhub-creds with the Docker Hub username + access token, ID exactly dockerhub-creds. Also the repo name must be library-system.
- Deploy stage: "The connection to the server localhost:8080 was refused" or kubectl can't connect
                                    -> Minikube is not running for the jenkins user. Do Part 5 again (sudo su - jenkins -s /bin/bash, minikube start).
- Pod shows ImagePullBackOff        -> the Docker Hub repo is Private (make it Public) or the push failed.
- Pod shows CrashLoopBackOff        -> run  kubectl logs deploy/library-app  and send me the output.
- Site shows old version after build 2 -> restart the port-forward and press Ctrl+F5.
- Out of memory / server frozen     -> you used a smaller instance than t3.medium.

=====================================================================
VIVA CHEAT SHEET (short answers)
=====================================================================
Q: What is CI/CD?     A: CI = every code change is automatically built and tested. CD = it is automatically delivered/deployed.
Q: What does Jenkins do here?  A: Runs the Jenkinsfile: checkout, build/test, docker build, push, deploy, verify, with no manual steps.
Q: What is a Jenkinsfile?  A: A text file in the repo that defines the pipeline as code (declarative pipeline with stages).
Q: What is a Dockerfile / multi-stage build?  A: Recipe for the image. Stage 1 compiles the C++; stage 2 copies only the compiled program into a small image.
Q: Why Docker Hub?    A: A registry. Kubernetes pulls the image from there, so the image must be pushed first.
Q: Why tag with the build number?  A: Every version is traceable and rollback is possible; "latest" alone would not show what changed.
Q: Deployment vs Service?  A: Deployment keeps the pod running and handles rolling updates. Service gives a stable address (NodePort 30080) to reach the pod.
Q: What is Minikube?  A: A single-node Kubernetes cluster for learning/testing.
Q: Where are credentials stored?   A: In Jenkins credentials store (id dockerhub-creds), never in the code.
Q: What is your "test" stage?  A: Compile with warnings on, then a smoke test that starts the server and checks health, catalog loading, issue/return rules and the page.
Q: What happens on the second run?  A: New commit -> new build number -> new image -> kubectl apply changes the image tag -> pod replaced with zero config change by hand.
Q: Limitation?  A: Book data is held in the pod (reset on restart); one replica; no authentication.
