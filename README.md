# myShell 프로젝트

**제출일** : 2021-12-12

                                                                                                    **유동현**(12171652)

# 1. **요구사항 정의**

### 1) cd 명령어

### 2) exit 명령어

### 3) & background 기능

### 4) SIGCHLD signal 처리

### 5) SIGINT, SIGTSTP signal 처리

### 6) Redirection 구현

### 7) Pipe 구현

---

# 2. 구현방법

![시그널 처리 완료했을 때의 main 코드](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/5bb1ea81-6cc3-4a01-b6f7-4405aa7f29e5/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-11-16_21-32-07.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183040Z&X-Amz-Expires=86400&X-Amz-Signature=5491085b0f4c0376dc3bb42537e1857856f964ac0a4a9609c0cc75c32c313952&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-11-16%252021-32-07.png%22&x-id=GetObject)
[시그널 처리 완료했을 때의 main 코드]

---

### 1) cd 명령어

: 현재 실행되고 있는 쉘 프로세스의 작업 디렉토리가 변경되어야 합니다. 따라서 cd 명령어가 입력되면 `fork()`를 하지 않고 `chdir()`을 호출합니다.

---

### 2) exit 명령어

 : 현재 실행되고 있는 쉘 프로세스가 종료되어야 합니다. 따라서 `exit` 명령어가 입력되면 `fork()`를 하지 않고 `exit(0)`을 호출하여 정상 종료 합니다.

---

### 3) & background

 : 명령어에 & 기호가 끝에 포함되어 있으면 background 프로세스라고 판단합니다. 해당 명령은 실행 후, `wait` 하지 않습니다. `wait()` 함수를 사용하게 되면, 쉘이 block 되므로 백그라운드 작업이 될 수 없습니다. 또 background 프로세스 종료 시그널에 의해 foreground에서 정상 종료 되어야하는 프로세스가 좀비 프로세스가 될 수 있습니다. 그래서 `waitpid()`를 통해 foreground 프로세스 id를 지정해주었습니다.

- [고찰1] : 이 방식은 background로 실행한 프로세스를 부모 프로세스가 받아주지 않기 때문에 무조건 백그라운드로 실행이 되고, 실행이 끝나면 좀비 프로세스가 됩니다. 따라서 추후 SIGCHLD handler를 통해 적절히 처리해주어야 합니다.
- [고찰2] : 주어진 테스트 방식에서의 문제점 1) (백그라운드가 잘 작동하지 않을 때) 백그라운드로 실행했지만, 포그라운드에서 10초가 실행되고, 그 뒤에 20 초 sleep을 실행시킬 수 있습니다. 이 때는 ps를 출력해서 그 결과를 보는 것이 의미가 없습니다. 따라서 먼저 실행시키는 프로세스의 작동 시간이 훨씬 길어야 합니다. 2) ps 명령어만으로는 ppid를 확인할 수 없습니다. Ppid 가 쉘의 pid로 동일해야 하기 때문에, IPC 일종인 pipe를 이용하여 ps -ef | grep sleep 명령어로 ppid가 쉘의 pid와 동일한지를 확인해야 합니다.

---

### 4) SIGINT & SIGTSTP 처리

![스크린샷, 2021-11-16 21-33-39.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/0699f6db-f08d-44e3-a31b-2833dabe140d/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-11-16_21-33-39.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183147Z&X-Amz-Expires=86400&X-Amz-Signature=a6025e6c6aa395e13e046b748ae08456da8df8fb04535d2b52ccb38bfd829aad&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-11-16%252021-33-39.png%22&x-id=GetObject)

 : 사용자가 `ctrl+c`, `ctrl+z` 를 입력하게 되면, pid가 0인 루트 프로세스가 현재 터미널 창에 있는 모든 foreground 프로세스에 해당 시그널을 보내는 것을 확인했습니다.  쉘 프로세스에게 먼저 시그널이 도착하고, 그 뒤에 쉘에서 생성된 자식 프로세스에게 도착합니다. 부모 프로세스(쉘)는 시그널을 받게 되면 SIGINT+SIGTSTP handler를 통해 프로세스가 종료되지 않게 처리합니다. 자식 프로세스는 `exec`로 만들어졌기 때문에 시그널 핸들러를 물려받지 못하여, 사용자의 의도대로 바로 종료됩니다. 

    사용자가 `ctrl+c`, `ctrl+z` 을 입력하는 시점은 (1) main:84 의 `fgets()`이 호출되어 사용자 입력을 기다릴 때, (2) foreground 프로세스가 실행 중일 때 입니다. 

    - (1) 이 때 표준 입력을 받는 system call 이 호출되고 block 되어 있는데, signal 을 통해 interrupt 받게 되면 error = EINTR로 설정되고 read system call 이 종료됩니다. signal handler 가 다 수행되고 나면 다음 라인인 main:85번 줄부터 다시 실행됩니다. 뒤 부분은 사용자로부터 정상 입력을 받았다고 가정하여 작성한 코드이므로 문제가 생길 여지가 있습니다. 따라서 SIGINT/SIGTSTP handler 내부에서 `cmdline[0]`에 터미널 문자를 삽입하여 입력이 없다고 인식하게 하였고,  버퍼가 비어있으면 while 문 처음부터 쉘을 수행하도록 main:88 continue를 삽입하였습니다.

    - (2) main:110 `wait` 에서 기다리던 중 interrupt 되면 `wait 함수는 중단 ⇒ 시그널 핸들러 ⇒ main:84 fgets() 기다림` ⇒ 정상적인 행동

---

### 5) SIGCHLD 처리

![스크린샷, 2021-11-16 21-33-57.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/7c0cd47e-c524-490c-9c59-ca57f9ef0f0e/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-11-16_21-33-57.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183210Z&X-Amz-Expires=86400&X-Amz-Signature=f16987dab9750d8a2431bdb9bcb863252107be689f75e311bf19e04a69d0d01c&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-11-16%252021-33-57.png%22&x-id=GetObject)

    : foreground process 가 정상 종료되어 main:110 waitpid() 에 의해 처리되는 경우를 제외하면, 모두 SIGCHLD handler 에 의해 처리됩니다. 즉, background & interrupted foreground 프로세스들이 해당됩니다. SIGCHLD를 처리하면서 가장 중요하게 생각했던 점은 main:75에서 `sa_restart` flag를 설정한 것입니다. 그 이유는 아래와 같습니다.

![sa_restart를 설정하지 않았을 때, myshell 프롬프트가 두번 연속으로 출력되는 것을 볼 수 있음.](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/7fabffc7-7d57-475b-8e1a-19175ab249dc/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-11-16_22-45-14.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183239Z&X-Amz-Expires=86400&X-Amz-Signature=875e54f90f5ff2734f6b676ef68e72fcb022ab5efe69c9e462e8ec44169bd73d&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-11-16%252022-45-14.png%22&x-id=GetObject)
[sa_restart를 설정하지 않았을 때, myshell 프롬프트가 두번 연속으로 출력되는 것을 볼 수 있음.]

    - (1) 쉘은 background 프로세스의 SIGCHLD 에 영향을 받으면 안됩니다. 만약 main:84 fgets() 가 대기하던 중 SIGCHLD 가 발생하면, read system call 이 종료되고 다시 while 처음으로 돌아가서 쉘 프롬프트를 한번 더 출력하게 됩니다. 이는 사용자에게 생뚱맞은 프롬프트 출력으로 보입니다. 따라서 fgets() 가 시그널에 의해 종료되었을 때, 다시 시작하도록 handler flag 에 `sa_restart` 를 설정해주었습니다.

   - (2) interrupted foreground process를 처리할 때도 마찬가지로 sa_restart를 설정해주어야만 합니다. interrupt 되면 쉘 프로세스가 `main:110 waitpid() ⇒ main:82 while() ⇒ 프롬프트 출력 ⇒ fgets() 대기` 상태로 접어듭니다. 후에 자식 프로세스가 interrupt 되어 종료되는데, 이 때 쉘이 SIGCHLD를 받습니다. 쉘은 현재 `fgets()` 에서 대기 중이기 때문에 re_restart를 설정하지 않으면 프롬프트가 연속 2번 출력되는 효과를 낳습니다. 

---

### < 3차시 이후 최종 main 함수>

![스크린샷, 2021-12-12 02-34-09.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/8ba1c6c4-a959-4acb-92fe-68e5f5ea3994/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-34-09.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183257Z&X-Amz-Expires=86400&X-Amz-Signature=0da3eec083a5a016fda45439715668e869c0b54824d22dbdd342dd56ce9b9e22&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-34-09.png%22&x-id=GetObject)

---

### 6) Redirection 구현

![[main 함수] redirection 을 위해 271-278 라인이 추가됨.](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/3fba14e8-1507-4c0c-9d5f-6d767bc18402/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-34-58.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183315Z&X-Amz-Expires=86400&X-Amz-Signature=16f7c9c58fb119ecc7c693d5ac1d2f1c4c59af0805f86d675f6c73901474ff95&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-34-58.png%22&x-id=GetObject)
[[main 함수] redirection 을 위해 271-278 라인이 추가됨.]

 기본적인 pipe가 없는 형태의 redirection을 먼저 구현했습니다. 기존의 `cmdvector` 에서 redirection character 가 존재하면 redirection을 만들어주었습니다. 쉘 프로세스의 `stdout` 과 `stdin` 의 file descripter 는 변경되면 안되므로, 위의 작업은 fork된 자식 프로세스에서 진행하였습니다.

![redirection 이 존재하는지 검사하는 함수. 개수를 반환한다.](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/c501dc28-7157-4ee9-8d13-b70ba7f8d364/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-13-57.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183336Z&X-Amz-Expires=86400&X-Amz-Signature=eef17b6c6357cdf85b51171fb4aa6db0f206f18f4c93df08ed094e0b6807893d&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-13-57.png%22&x-id=GetObject)
[redirection 이 존재하는지 검사하는 함수. 개수를 반환한다.]

 redirection character 가 존재하는지 검사하는 함수입니다. `mode` 에는 `"<"`, `">"` 둘 중 하나가 선택됩니다. 해당 문자를 cmdvector 에서 발견하면 인덱스를 반환합니다. 인덱스는 redirection을 만드는 함수에게 전달되어 파일 이름을 추출한 후, 해당 문자와 파일 이름을 `cmdvector`에서 제거할 때 사용됩니다.

![redirection 을 만드는 함수](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/e0852468-409f-4083-8f03-68beef9ca345/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-17-06.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183341Z&X-Amz-Expires=86400&X-Amz-Signature=3488a773a839994c24afea3c99d1bcf14b27b1a486fba0467f127f6963bd7715&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-17-06.png%22&x-id=GetObject)
[redirection 을 만드는 함수]

  `cmdvector` 에서 파일 이름을 찾아서 해당 파일을 open 한 후에, `"<"` 또는 `">"` 문자에 맞추어 적절히 redirection 을 만들어 줍니다. redirection 을 성공적으로 만든 후에는 해당 파일 descripter를 닫고, `cmdvector` 에서 파일 이름과 `"<"` 또는 `">"`을 지웁니다.

![redirection character 와 filename 을 지우는 함수](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/979801e2-4d16-4837-bf0f-19e9be584091/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-17-15.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183344Z&X-Amz-Expires=86400&X-Amz-Signature=998b9558dbe1b5055f7e361f89fea170c52b514b3bcde28e3667e1d3356e97dc&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-17-15.png%22&x-id=GetObject)
[redirection character 와 filename 을 지우는 함수]

  

---

### 7) Pipe 구현

![파이프 문자 개수만큼 파이프를 생성. 위처럼 파이프를 사용.](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/ccdeecad-cd9b-4c99-8a19-81b8a440b96d/ds.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183346Z&X-Amz-Expires=86400&X-Amz-Signature=66e3895bf8b6284cdb5f74e7efdd81ccf918279d8b1759681c47b59686af8f55&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22ds.png%22&x-id=GetObject)
[파이프 문자 개수만큼 파이프를 생성. 위처럼 파이프를 사용.]

![[main 함수] pipe를 위해 265-269 라인이 추가됨. doPipe() 함수가 핵심.](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/3fba14e8-1507-4c0c-9d5f-6d767bc18402/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-34-58.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183348Z&X-Amz-Expires=86400&X-Amz-Signature=df8fcab20d29442aafa1286f4445e1ece645bb0388e16737db5020a3504d44b0&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-34-58.png%22&x-id=GetObject)
[[main 함수] pipe를 위해 265-269 라인이 추가됨. doPipe() 함수가 핵심.]

![파이프 문자의 개수를 반환한다.](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/bb6d9b91-4df6-4307-bd5d-d479f0bdb2c5/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-14-08.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183351Z&X-Amz-Expires=86400&X-Amz-Signature=a671bb222397673e8ded977f2a6450c152267f4896bb57b123832530d10f4cea&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-14-08.png%22&x-id=GetObject)
[파이프 문자의 개수를 반환한다.]

 `cmdvector` 에서 pipe 문자의 개수를 세고, pipe 가 있으면 child에게 `doPipe()`를 실행시킨다. 쉘은 자식 프로세스가 성공적으로 파이프 명령들을 수행하기를 기다린다. (백그라운드 실행일 때는 예외)

**<doPipe 함수>**

1. 지역변수 선언부
    
    ![스크린샷, 2021-12-12 02-42-08.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/9525a11c-d0ee-4c6b-92c0-1c0b22d66cf2/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-42-08.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183353Z&X-Amz-Expires=86400&X-Amz-Signature=2e1dfe686b8e248a0a4f161619e01f36d40026d84835384c2673191c45548270&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-42-08.png%22&x-id=GetObject)
    
2. 첫 번째 파이프 실행 부분
    - `129-137 라인` : redirection 처리 부분. 위의 로직과 동일.
    - `138 라인` : 위에서 선언한 `cmd_idx를` 바탕으로 현재 실행할 pipe 의 명령어만 추출
    - `141-160 라인` : child 에게 pipe를 실행시킴. 부모는 child를 기다림. 둘 다 필요 없는 fd를 닫아줌.
    
    ![스크린샷, 2021-12-12 02-43-55.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/2c43e7cd-9e8f-4b05-b3a6-e747842c563b/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-43-55.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183355Z&X-Amz-Expires=86400&X-Amz-Signature=4bed41bcc4ee66b800db51e2a895b5f0285bade7c8dca20c8a736b751f596181&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-43-55.png%22&x-id=GetObject)
    
3. 중간 파이프 실행 부분
    - 마지막 pipe 명령 빼고 전부 실행시킴
    - `165-166 라인` : 현재 실행시킬 파이프 명령만 추출
    - `170-186 라인` : 자식 프로세스에게 파이프 실행시키고 부모는 기다림. 둘 다 필요 없는 fd 닫음.
    
    ![스크린샷, 2021-12-12 02-48-17.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/98e645cc-4deb-4445-8e63-5bbb84f72395/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-48-17.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183358Z&X-Amz-Expires=86400&X-Amz-Signature=746a7708bcc0de6bb01704827077fb17d965d18f98646de2d43848c1705e4e0e&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-48-17.png%22&x-id=GetObject)
    
4. 마지막 파이프 실행부분
    - `188-196 라인` : 리다이렉션 부분, 언급했던 로직과 동일함.
    - `197 라인` : 파이프 명령만 추출
    - `199-217 라인` : 자식에게 파이프 실행시키고 부모는 기다림. 둘 다 필요 없는 fd 닫음.
    
    ![스크린샷, 2021-12-12 02-50-51.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/e3cd2716-01bd-4515-8dba-2d4290c1b8d4/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_02-50-51.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183400Z&X-Amz-Expires=86400&X-Amz-Signature=5580c8a3b22b2bdac776dfa0ce141f11d3c11b83571ea0f986c19e18e3184281&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252002-50-51.png%22&x-id=GetObject)
    

---

# 3. Pseudo Code

### 1) cd

```c
/* chdir */
if (i > 0 && strcmp(cmdvector[0], "cd")==0){
        chdir(cmdvector[1]);
}
```

### 2) exit

```c
/* exit */
if (i > 0 && strcmp(cmdvector[0], "exit")==0){
        return 0;
}
```

### 3) & background

```c
/* background */
int bg_flag = (i>0 && strcmp("&", cmdvector[i-1])==0 ? 1 : 0);

switch(pid=fork()){
case 0:
        if (bg_flag) cmdvector[i-1] = '\0';     /* background */

        execvp(cmdvector[0], cmdvector);
        fatal("main()");
case -1:
        fatal("main()");
default:
        if (!bg_flag) waitpid(pid, NULL, 0);
```

### 4) SIGINT&SIGTSTP handler

```c
/* SIGINT & SIGTSTP */
void interrupt_handler(int sig_no, siginfo_t *sig_info, void* param2){
    pid_t who_signal = sig_info->si_pid;

    /* root process(pid==0) sends signals to this shell */
    if (who_signal == 0) {
            printf("\n");
            cmdline[0] = '\0';
    }
}

// main...
/* SIGINT & SIGTSTP handler*/
static struct sigaction act;
act.sa_sigaction = interrupt_handler;
act.sa_flags = SA_SIGINFO;
sigfillset(&act.sa_mask);
sigaction(SIGINT, &act, NULL);
sigaction(SIGTSTP, &act, NULL);
```

### 5) SIGCHLD handler

```c
/* SIGCHLD */
void child_handler(int sig_no){
        while(waitpid(-1, NULL, WNOHANG)>0);
}

// main...
/* SIGCHLD handler */
static struct sigaction child;
child.sa_handler = child_handler;
child.sa_flags = SA_RESTART;
sigfillset(&child.sa_mask);
sigaction(SIGCHLD, &child, NULL);
```

### 6) Redirection

```c
int makeRedirection(char** cmd, int char_idx, char* mode) {
    int fd, i;
    char* filepath = cmd[char_idx + 1];

    if (strcmp("<", mode) == 0) { /* infile redirection */
        if ((fd = open(filepath, O_RDONLY)) == -1) {
            perror("syscall 'open' failed!!");
            return -1;
        }
        dup2(fd, STDIN_FILENO);
    }
    else if (strcmp(">", mode) == 0) { /* outfile redirection */
        if ((fd = open(filepath, O_WRONLY|O_TRUNC|O_CREAT, 0644)) == -1) {
            perror("syscall 'open' failed!!");
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
    }
    
    _removeRedirectionChar(cmd, char_idx);
    
    close(fd);

    return 0;
}
```

```c
int doesRedirectionExist(char** cmd, char* mode) {
    for (int i = 0; cmd[i] != NULL; i++)
        if (strcmp(cmd[i], mode) == 0) return i;

    return -1;
}
```

```c
void _removeRedirectionChar(char** cmd, int idx){
    int i;
    for (i = idx; cmd[i+2] != NULL; i++)
        cmd[i] = cmd[i+2];
    cmd[i] = NULL;
}
```

### 7) Pipe

```c
int DoPipesExist(char** cmd){
    int cnt = 0;
    for (int i = 0; cmd[i] != NULL; i++)
        if (strcmp(cmd[i], "|") == 0)
            cnt += 1;

    return cnt;
}
```

```c
void _getSubCmd(char* cmd[MAX_CMD_ARG], char* subcmd[MAX_CMD_ARG], int *cmd_idx){
    int i;
    for(i = 0; cmd[(*cmd_idx)] != NULL && strcmp(cmd[(*cmd_idx)], "|") != 0 ; i++, (*cmd_idx)++)
        subcmd[i] = cmd[(*cmd_idx)];

    subcmd[i] = NULL;
}
```

```c
void doPipe(char** cmd, int pipe_cnt){
    int pipe_fd[MAX_CMD_ARG][2];
    char* subcmd[MAX_CMD_ARG];
    char* filepath;
    int fd;
    int in_idx, out_idx;
    int cmd_idx = 0;
    pid_t pid;

    // first cmd
    pipe(pipe_fd[0]);
    if((in_idx = doesRedirectionExist(cmd, "<")) > -1){
        filepath = cmd[in_idx + 1];
        if ((fd = open(filepath, O_RDONLY)) == -1) {
            perror("syscall 'open' failed!!");
            return;
        }
        _removeRedirectionChar(cmd, in_idx);
    }
    _getSubCmd(cmd, subcmd, &cmd_idx); /* extract pipe cmd */

    if ((pid = fork()) == -1) fatal("fail to fork for pipe!");
    else if (pid == 0) {
        if (in_idx > -1) {
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        dup2(pipe_fd[0][1], STDOUT_FILENO);
        close(pipe_fd[0][0]);
        close(pipe_fd[0][1]);

        execvp(subcmd[0], subcmd);
        fatal("doing pipe!1");
    }
    else {
        if (in_idx > -1) close(fd);
        close(pipe_fd[0][1]);
        waitpid(pid, NULL, 0);
    }

    // middle cmd
    int cur_cnt = 0;
    while(++cur_cnt < pipe_cnt){
        pipe(pipe_fd[cur_cnt]);
        cmd_idx += 1;
        _getSubCmd(cmd, subcmd, &cmd_idx);

        if((pid = fork()) == -1)
            fatal("fail to fork for pipe!");
        else if (pid == 0) {
            dup2(pipe_fd[cur_cnt - 1][0], STDIN_FILENO);
            dup2(pipe_fd[cur_cnt][1], STDOUT_FILENO);
            close(pipe_fd[cur_cnt - 1][0]);
            close(pipe_fd[cur_cnt][0]);
            close(pipe_fd[cur_cnt][1]);

            execvp(subcmd[0], subcmd);
            fatal("doing pipe!2");
        }
        else {
            close(pipe_fd[cur_cnt -1][0]);
            close(pipe_fd[cur_cnt][1]);
            waitpid(pid, NULL, 0);
        }
    }

    // last cmd
    if((out_idx = doesRedirectionExist(cmd, ">")) > -1){
        filepath = cmd[out_idx + 1];
        if ((fd = open(filepath, O_WRONLY|O_TRUNC|O_CREAT, 0644)) == -1) {
            perror("syscall 'open' failed!!");
            return;
        }
        _removeRedirectionChar(cmd, out_idx);
    }
    cmd_idx += 1;
    _getSubCmd(cmd, subcmd, &cmd_idx); /* extract pipe cmd */

    if ((pid = fork()) == -1)
        fatal("fail to fork for pipe!");
    else if (pid == 0) {
        if (out_idx > -1) {
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        dup2(pipe_fd[cur_cnt - 1][0], STDIN_FILENO);
        close(pipe_fd[cur_cnt - 1][0]);

        execvp(subcmd[0], subcmd);
        fatal("doing pipe!3");
    }
    else {
        if (out_idx > -1) close(fd);
        close(pipe_fd[cur_cnt -1][0]);
        waitpid(pid, NULL, 0);
    }
}
```

---

# 4. 실행결과 캡쳐

### 1) background

![스크린샷, 2021-11-16 22-57-36.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/2baeae62-efae-4344-8424-fb0aa3be4635/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-11-16_22-57-36.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183740Z&X-Amz-Expires=86400&X-Amz-Signature=6946cad681527188e8aa30625a00973a3f0dd2580145ed6b9cf7d96cf8c59606&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-11-16%252022-57-36.png%22&x-id=GetObject)

### 2) SIGINT & SIGTSTP 쉘 무시

![스크린샷, 2021-11-16 22-17-40.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/b2450ae3-e4f1-46e0-b814-059e4353a746/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-11-16_22-17-40.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183745Z&X-Amz-Expires=86400&X-Amz-Signature=19622414100709ccd8cdf939f4a93f8f9dce28e0ee51f09755787f305f533496&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-11-16%252022-17-40.png%22&x-id=GetObject)

### 3) 쉘의 SIGCHLD 처리 & 자식 프로세스의 SIGINT/SIGTSTP

![스크린샷, 2021-11-16 22-18-27.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/deea4756-8a7a-41fa-b8c1-6d363bdfef16/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-11-16_22-18-27.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183748Z&X-Amz-Expires=86400&X-Amz-Signature=6d121fe3d9fef66fe6b7335863f5ea23539927895e39d55817b3cd995d4939d2&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-11-16%252022-18-27.png%22&x-id=GetObject)

### 4) Redirection Test

![스크린샷, 2021-12-12 03-11-20.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/c8c87b3f-5456-4709-9659-a53702ad297d/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_03-11-20.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183750Z&X-Amz-Expires=86400&X-Amz-Signature=9aa1e2a9fe9f1e6009e92fbee7a19651dbe5219915f2d0653a49ae5141ea55c4&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252003-11-20.png%22&x-id=GetObject)

### 5) Pipe & Redirection Test

![스크린샷, 2021-12-12 03-12-22.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/196a2b03-669a-422c-afef-6ba3b954e9d0/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_03-12-22.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183752Z&X-Amz-Expires=86400&X-Amz-Signature=e9192c7ac00f833ac99bebf1d4551086eac7f15c5fca7dd745b89f7650923bbb&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252003-12-22.png%22&x-id=GetObject)

### 6) Background Test

![스크린샷, 2021-12-12 03-13-28.png](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/36b97c59-3a40-4768-aac7-e73bad0dd9a3/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7_2021-12-12_03-13-28.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20211211%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20211211T183754Z&X-Amz-Expires=86400&X-Amz-Signature=8aec4a40f217a9fb31e97bf577e9373a2837649b86642be90a00f4c4942806bb&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7%252C%25202021-12-12%252003-13-28.png%22&x-id=GetObject)
