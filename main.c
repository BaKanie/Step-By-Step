#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_TASKS 5  // 최대 등록 개수
#define DATA_FILE "todo.dat" // 할 일 저장 파일 이름

// 할 일 하나의 정보 모음 구조체
struct Task {
    char title[100];     // 제목
    char deadline[30];   // 마감 문자열
    char steps[3][100];  // 세부 단계 3개 (=step)
    int complete[3];     // 0: 미완료, 1: 완료 (각 단계의 완료 여부)
    time_t due;          // 날짜 비교용 시간 값
};

// 한 줄 입력받기 함수
// text: 입력 저장 배열, size: 배열 크기
// allowEmpty: 1이면 빈 Enter 허용, 0이면 재입력
int readLine(char text[], int size, int allowEmpty)
{
    while (1) {
        if (fgets(text, size, stdin) == NULL) {
            printf("(ERROR) Could not read input.\n");
            return 0;
        }

        char *newline = strchr(text, '\n'); // 줄바꿈 위치

        if (newline != NULL) {
            *newline = '\0';
        } else {
            int ch = getchar(); // 저장 공간 다음의 입력 문자

            if (ch != '\n' && ch != EOF) {
                // 저장 공간을 초과한 입력 비우기
                while ((ch = getchar()) != '\n' && ch != EOF) {
                }

                printf(
                    "(ERROR) Input too long. Enter again "
                    "(maximum %d bytes; English up to %d characters): ",
                    size - 1, size - 1
                );
                continue;
            }
        }

        if (!allowEmpty && text[0] == '\0') {
            printf("(ERROR) This field cannot be empty. Enter again: ");
            continue;
        }

        return 1;
    }
}

// 예아니오 질문 함수
// question: 화면에 표시할 질문
// YES는 1, NO는 0, 입력 실패는 -1 반환
int askYesNo(const char question[])
{
    char answer[20]; // YES 또는 NO 입력 저장

    while (1) {
        printf("%s (YES or NO): ", question);

        if (!readLine(answer, sizeof(answer), 0)) {
            return -1;
        }

        if (strcmp(answer, "YES") == 0 ||
            strcmp(answer, "yes") == 0) {
            return 1;
        }

        if (strcmp(answer, "NO") == 0 ||
            strcmp(answer, "no") == 0) {
            return 0;
        }

        printf("(ERROR) Please enter YES or NO.\n");
    }
}

// 마감문자열을 비교가능한 시간 값으로 변환하기 함수
// text: 마감 문자열, due: 변환 결과를 저장할 주소
// 유효한 마감이면 1, 잘못된 입력이면 0 반환
int transDeadline(const char text[], time_t *due)
{
    int year, month, day, hour, minute; // 입력에서 읽을 날짜와 시간
    char formatted[30];                // 형식 비교용 문자열

    if (sscanf(text, "%4d-%2d-%2d %2d:%2d",
               &year, &month, &day, &hour, &minute) != 5) {
        return 0;
    }

    // YYYY-MM-DD HH:MM 형식인지 확인
    snprintf(formatted, sizeof(formatted),
             "%04d-%02d-%02d %02d:%02d",
             year, month, day, hour, minute);

    if (strcmp(text, formatted) != 0) {
        return 0;
    }

    if (year < 1970 || year > 9999 ||
        month < 1 || month > 12 ||
        day < 1 || day > 31 ||
        hour < 0 || hour > 23 ||
        minute < 0 || minute > 59) {
        return 0;
    }

    struct tm date = {0}; // 날짜와 시간을 담을 표준 구조체

    date.tm_year = year - 1900;
    date.tm_mon = month - 1;
    date.tm_mday = day;  // day of month: 라이브러리에 정해진 이름
    date.tm_hour = hour;
    date.tm_min = minute;

    time_t result = mktime(&date); // 날짜를 비교 가능한 시간 값으로 변환

    if (result == (time_t)-1) {
        return 0;
    }

    // 2월 30일처럼 자동 보정된 잘못된 날짜 제외
    if (date.tm_year != year - 1900 ||
        date.tm_mon != month - 1 ||
        date.tm_mday != day ||
        date.tm_hour != hour ||
        date.tm_min != minute) {
        return 0;
    }

    *due = result;
    return 1;
}

// 할 일 제목, 마감, step 입력받기 함수
// task: 입력할 할 일 구조체의 주소
int inputTask(struct Task *task)
{
    printf("Title: ");

    if (!readLine(task->title, sizeof(task->title), 0)) {
        return 0;
    }

    while (1) {
        printf("Deadline (0000-00-00 00:00): ");

        if (!readLine(task->deadline, sizeof(task->deadline), 0)) {
            return 0;
        }

        if (transDeadline(task->deadline, &task->due)) {
            break;
        }

        printf(
            "(ERROR) Invalid date or format. "
            "Please try again (0000-00-00 00:00).\n"
        );
    }

    for (int i = 0; i < 3; i++) { // i: Step의 배열 위치
        printf("Step %d (Enter to leave blank): ", i + 1);

        // Step은 내용 없이 Enter만 입력해도 허용
        if (!readLine(task->steps[i], sizeof(task->steps[i]), 1)) {
            return 0;
        }

        task->complete[i] = 0;
    }

    return 1;
}

// 선택 정렬로 마감이 빠른 순서대로 배열 정리 함수
// todo: 할 일 배열, count: 실제 등록 개수
void sortTasks(struct Task todo[], int count)
{
    for (int i = 0; i < count - 1; i++) { // i: 정렬할 위치
        int earliest = i; // 가장 빠른 마감 항목의 위치

        for (int j = i + 1; j < count; j++) { // j: 비교할 위치
            if (difftime(todo[j].due, todo[earliest].due) < 0) {
                earliest = j;
            }
        }

        // 제목과 Step 등을 포함한 구조체 전체 교환
        if (earliest != i) {
            struct Task temp = todo[i]; // 교환 중 임시 보관
            todo[i] = todo[earliest];
            todo[earliest] = temp;
        }
    }
}

// 마감된 항목을 삭제하고 남은 개수 반환 함수
// todo: 할 일 배열, count: 등록 개수, now: 현재 시각
int deleteExpired(struct Task todo[], int count, time_t now)
{
    int remaining = 0; // 남은 개수이자 다음 저장 위치

    for (int i = 0; i < count; i++) { // i: 검사할 항목 위치
        if (difftime(todo[i].due, now) > 0) {
            // 남길 항목을 배열 앞쪽으로 모으기
            todo[remaining] = todo[i];
            remaining++;
        } else {
            printf("Deleted: %s\n", todo[i].title);
        }
    }

    return remaining;
}

// 등록된 할 일 전체 출력 함수
// todo: 출력할 할 일 배열, count: 등록 개수
void printTasks(const struct Task todo[], int count)
{
    printf("\n==== Your Tasks (%d/%d) ====\n", count, MAX_TASKS);

    for (int i = 0; i < count; i++) { // i: 출력할 할 일 위치
        printf("\nTask %d\n", i + 1);
        printf("Title: %s\n", todo[i].title);
        printf("Deadline: %s\n", todo[i].deadline);

        for (int j = 0; j < 3; j++) { // j: 출력할 Step 위치
            printf("Step %d: %s\n", j + 1, todo[i].steps[j]);
        }
    }
}

// 할 일 저장 함수
// todo: 저장할 배열, count 실제 등록 개수
// 성공하면 1, 실패하면 0 반환
int saveTasks(const struct Task todo[], int count)
{
    FILE *file = fopen(DATA_FILE, "wb"); //이진 파일 쓰기

    if (file == NULL) {
        printf("(ERROR) Could not open the save file.\n");
        return 0;
    }

    // 개수를 저장한 다음 배열 전체를 한 묶음으로 저장
    if (fwrite(&count, sizeof(count), 1, file) != 1 ||
        fwrite(todo, sizeof(struct Task) *MAX_TASKS, 1, file) != 1) {
        printf("(ERROR) Could not save tasks. \n");
        fclose(file);
        return 0;
        }

        if (fclose(file) != 0) {
            printf("(ERROR) Could not finish saving.\n");
            return 0;
        }

        printf("Saved %d task(s).\n", count);
        return 1;
}

// 파일 불러오기 함수
// todo: 복원할 배열, count: 등록개수를 저장할 주소
// 성공하면 1, 실패하거나 파일이 없으면 0 반환
int loadTasks(struct Task todo[], int *count)
{
    FILE *file = fopen(DATA_FILE, "rb"); // 이진 파일 읽기

    if (file == NULL) {
        printf("Could not open a saved file. \n");

        // 파일이 없는 첫 실행인지 다른 오류인지는 구분하지 않는다
        // 빈 목록으로 시작할지 사용자에게 확인
        return askYesNo("Start a new list?") == 1;
    }

    int savedCount = 0; // 파일에 저장된 등록 개수
    struct Task loaded[MAX_TASKS] = {0}; // 읽은 내용을 임시 보관

    // 저장할 때와 같은 순서와 크기로 읽기
    if (fread(&savedCount, sizeof(savedCount), 1, file) != 1 ||
    savedCount < 0 || savedCount > MAX_TASKS ||
    fread(loaded, sizeof(loaded), 1, file) != 1) {
    printf("(ERROR) Could not read saved tasks.\n");
    fclose(file);
    return 0;
    }

    fclose(file);

    for (int i = 0; i < savedCount; i++) { //복원할 항목 위치
        todo[i] = loaded[i];
    }

    *count = savedCount;
    printf("Loaded %d task(s).\n", *count);
    return 1;
}

// 프로그램 메인 함수
int main(void)
{
    struct Task todo[MAX_TASKS] = {0}; // 최대 5개의 할 일 저장 공간
    int count = 0;                    // 실제 등록 개수

    if (!loadTasks(todo, &count)) {
        return 1;
    }

    sortTasks(todo, count);
    printTasks(todo, count);

    if (count > 0) {
        int answer = askYesNo("Add another task?"); // 추가 등록 응답

        if (answer != 1) {
            return 0;
        }
    }

    while (1) {
        // 저장 공간이 가득 차면 마감 항목 삭제 여부 확인
        if (count == MAX_TASKS) {
            int answer = askYesNo( // 삭제 질문에 대한 응답
                "Storage is full. Delete expired tasks and add a new task?"
            );

            if (answer != 1) {
                break;
            }

            time_t now = time(NULL); // 현재 시각

            if (now == (time_t)-1) {
                printf("(ERROR) Could not get the current time.\n");
                break;
            }

            count = deleteExpired(todo, count, now);

            if (count == MAX_TASKS) {
                printf(
                    "(ERROR) No expired tasks. "
                    "Cannot add a new task.\n"
                );
                break;
            }
        }

        // 입력을 모두 마친 후 배열에 등록
        struct Task newTask = {0}; // 새 할 일 임시 저장 공간

        printf("\n--- Add Task ---\n");

        if (!inputTask(&newTask)) {
            break;
        }

        todo[count] = newTask;
        count++;

        sortTasks(todo, count);
        printTasks(todo, count);

        int answer = askYesNo("Add another task?"); // 추가 등록 응답

        if (answer != 1) {
            break;
        }
    }

        if (!saveTasks(todo, count)) {
        return 1;
    }

    printf("\nRegistration ended.\n");
    return 0;
}


/*포인트1
정보를 한 묶음으로 관리: struct Task, todo[MAX_TASKS]
구조체로 할 일 정보를 묶고 배열로 최대 5개를 관리한다. count는 실제 등록 개수다.*/

/*포인트2
정렬 시 정보 유지: struct Task temp = todo[i];부터 교환 부분
제목만 교환하지 않고 구조체 전체를 교환하므로 마감과 Step도 함께 이동한다*/

/*포인트3
종료 후 기록 복원: fwrite의 'fread'부분
등록 개수와 배열을 같은 순서, 같은 크기로 저장하고 읽어 복원한다.*/