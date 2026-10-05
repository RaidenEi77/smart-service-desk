// Smart Service Desk Simulator - CTDL & GT, Bai 12
// Bien dich: gcc smart_service_desk.c -o ssd
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===================== KIEU DU LIEU ===================== */
typedef struct Customer {
    char id[10];
    char name[50];
    char serviceType[20];
    int arrivalTime;      // phut tinh tu dau ngay
    int processingTime;   // pj (>= 1)
    int waitingWeight;    // wj
} Customer;

typedef struct Node {
    Customer data;
    struct Node* next;
} Node;

typedef struct Queue { Node* front; Node* rear; int size; } Queue; // hang doi FIFO
typedef struct List  { Node* head; int size; } List;               // danh sach tat ca khach

typedef struct Action {
    char type[10];        // ADD, DELETE, UPDATE
    Customer oldData;
    Customer newData;
} Action;

typedef struct SNode { Action data; struct SNode* next; } SNode;
typedef struct Stack { SNode* top; int size; } Stack;             // lich su (LIFO)

typedef struct Stats {
    int  clock;           // thoi diem hien tai cua quay
    int  served;
    long totalWait;
    long totalCost;       // tong wj * Cj
} Stats;

/* ===================== TIEN ICH ===================== */
Node* newNode(Customer c) {
    Node* p = (Node*)malloc(sizeof(Node));
    if (!p) { puts("Out of memory."); exit(1); }
    p->data = c;
    p->next = NULL;
    return p;
}

void readLine(const char* prompt, char* out, size_t size) {
    char buf[256];
    printf("%s", prompt);
    if (!fgets(buf, sizeof buf, stdin)) exit(0);   // het du lieu vao -> thoat
    buf[strcspn(buf, "\n")] = '\0';
    strncpy(out, buf, size - 1);
    out[size - 1] = '\0';
}

int readInt(const char* prompt, int minVal) {
    char buf[32], extra;
    int v;
    while (1) {
        readLine(prompt, buf, sizeof buf);
        if (sscanf(buf, "%d %c", &v, &extra) == 1 && v >= minVal) return v;
        printf("Invalid value (must be an integer >= %d). Try again.\n", minVal);
    }
}

void printCustomer(const Customer* c) {
    printf("%s | %s | %s | arrival=%d p=%d w=%d\n",
           c->id, c->name, c->serviceType,
           c->arrivalTime, c->processingTime, c->waitingWeight);
}

/* ===================== QUEUE (danh sach lien ket) ===================== */
int isEmpty(const Queue* q) { return q->front == NULL; }

// O(1)
void enqueue(Queue* q, Customer c) {
    Node* p = newNode(c);
    if (isEmpty(q)) q->front = q->rear = p;
    else { q->rear->next = p; q->rear = p; }
    q->size++;
}

// O(1). Tra ve 0 neu queue rong (khong lam chuong trinh crash)
int dequeue(Queue* q, Customer* out) {
    if (isEmpty(q)) return 0;
    Node* temp = q->front;
    *out = temp->data;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    q->size--;
    free(temp);
    return 1;
}

// O(n): xoa khach theo ma khoi hang doi
int removeFromQueue(Queue* q, const char* id) {
    Node *prev = NULL, *cur = q->front;
    while (cur && strcmp(cur->data.id, id) != 0) { prev = cur; cur = cur->next; }
    if (!cur) return 0;
    if (prev) prev->next = cur->next; else q->front = cur->next;
    if (cur == q->rear) q->rear = prev;
    q->size--;
    free(cur);
    return 1;
}

// O(n): cap nhat ban sao cua khach trong hang doi
void updateInQueue(Queue* q, Customer c) {
    for (Node* p = q->front; p; p = p->next)
        if (strcmp(p->data.id, c.id) == 0) { p->data = c; return; }
}

void printQueue(const Queue* q) {
    printf("Queue: ");
    if (isEmpty(q)) { puts("(empty)"); return; }
    for (Node* p = q->front; p; p = p->next)
        printf("%s%s", p->data.id, p->next ? " -> " : "\n");
}

void freeQueue(Queue* q) {
    Customer tmp;
    while (dequeue(q, &tmp)) {}
}

/* ===================== LIST (danh sach lien ket) ===================== */
// O(n): them cuoi danh sach (phai duyet den cuoi)
void addCustomer(List* l, Customer c) {
    Node* p = newNode(c);
    if (!l->head) l->head = p;
    else {
        Node* t = l->head;
        while (t->next) t = t->next;
        t->next = p;
    }
    l->size++;
}

// O(n): tim kiem tuyen tinh
Node* findCustomer(List* l, const char* id) {
    for (Node* p = l->head; p; p = p->next)
        if (strcmp(p->data.id, id) == 0) return p;
    return NULL;
}

// O(n): can tim phan tu truoc
int removeCustomer(List* l, const char* id) {
    Node *prev = NULL, *cur = l->head;
    while (cur && strcmp(cur->data.id, id) != 0) { prev = cur; cur = cur->next; }
    if (!cur) return 0;
    if (prev) prev->next = cur->next; else l->head = cur->next;
    free(cur);
    l->size--;
    return 1;
}

// O(n)
int updateCustomer(List* l, Customer c) {
    Node* p = findCustomer(l, c.id);
    if (!p) return 0;
    p->data = c;
    return 1;
}

void freeList(List* l) {
    Node* p = l->head;
    while (p) { Node* n = p->next; free(p); p = n; }
    l->head = NULL; l->size = 0;
}

/* ===================== STACK ===================== */
// O(1)
void push(Stack* s, Action a) {
    SNode* p = (SNode*)malloc(sizeof(SNode));
    if (!p) { puts("Out of memory."); exit(1); }
    p->data = a;
    p->next = s->top;
    s->top = p;
    s->size++;
}

// O(1). Tra ve 0 neu stack rong
int pop(Stack* s, Action* out) {
    if (!s->top) return 0;
    SNode* t = s->top;
    *out = t->data;
    s->top = t->next;
    s->size--;
    free(t);
    return 1;
}

void freeStack(Stack* s) {
    Action tmp;
    while (pop(s, &tmp)) {}
}

/* ===================== NHAP DU LIEU ===================== */
void inputDetails(Customer* c) {
    readLine("Name: ", c->name, sizeof c->name);
    readLine("Service type: ", c->serviceType, sizeof c->serviceType);
    c->arrivalTime    = readInt("Arrival time (minutes): ", 0);
    c->processingTime = readInt("Processing time pj (>=1): ", 1);
    c->waitingWeight  = readInt("Waiting weight wj (>=0): ", 0);
}

/* ===================== CHUC NANG ===================== */
void doAdd(List* list, Queue* q, Stack* history) {
    Customer c;
    memset(&c, 0, sizeof c);
    readLine("ID: ", c.id, sizeof c.id);
    if (c.id[0] == '\0') { puts("ID must not be empty."); return; }
    if (findCustomer(list, c.id)) { puts("ID already exists."); return; }
    inputDetails(&c);

    addCustomer(list, c);
    enqueue(q, c);

    Action a;
    memset(&a, 0, sizeof a);
    strcpy(a.type, "ADD");
    a.newData = c;
    push(history, a);

    printf("Added %s.\n", c.id);
    printQueue(q);
}

void doServe(Queue* q, Stats* st) {
    Customer c;
    if (!dequeue(q, &c)) { puts("Queue is empty - nobody to serve."); return; }

    int start  = st->clock > c.arrivalTime ? st->clock : c.arrivalTime;
    int finish = start + c.processingTime;          // Cj
    long cost  = (long)c.waitingWeight * finish;    // wj * Cj

    st->clock = finish;
    st->served++;
    st->totalWait += start - c.arrivalTime;
    st->totalCost += cost;

    printf("Served: %s | waitingCost = %ld\n", c.id, cost);
    printf("Remaining: ");
    printQueue(q);
}

void doSearchUpdate(List* list, Queue* q, Stack* history) {
    char id[10];
    readLine("ID to search: ", id, sizeof id);
    Node* p = findCustomer(list, id);
    if (!p) { puts("Customer not found."); return; }
    printCustomer(&p->data);

    char ans[8];
    readLine("Update this customer? (y/n): ", ans, sizeof ans);
    if (ans[0] != 'y' && ans[0] != 'Y') return;

    Customer old = p->data, nw = p->data;
    inputDetails(&nw);

    updateCustomer(list, nw);
    updateInQueue(q, nw);

    Action a;
    memset(&a, 0, sizeof a);
    strcpy(a.type, "UPDATE");
    a.oldData = old;
    a.newData = nw;
    push(history, a);
    puts("Updated.");
}

void doDelete(List* list, Queue* q, Stack* history) {
    char id[10];
    readLine("ID to delete: ", id, sizeof id);
    Node* p = findCustomer(list, id);
    if (!p) { puts("Customer not found."); return; }
    Customer old = p->data;

    if (!removeFromQueue(q, id)) {
        puts("Customer is not in the waiting queue (already served).");
        return;
    }
    removeCustomer(list, id);

    Action a;
    memset(&a, 0, sizeof a);
    strcpy(a.type, "DELETE");
    a.oldData = old;
    push(history, a);
    printf("Deleted %s.\n", id);
    printQueue(q);
}

void undo(Stack* history, List* list, Queue* q) {
    Action a;
    if (!pop(history, &a)) { puts("Nothing to undo."); return; }

    if (strcmp(a.type, "ADD") == 0) {
        if (removeFromQueue(q, a.newData.id)) {
            removeCustomer(list, a.newData.id);
            printf("Undo ADD: removed %s.\n", a.newData.id);
        } else {
            puts("Cannot undo ADD: customer was already served.");
        }
    } else if (strcmp(a.type, "DELETE") == 0) {
        if (findCustomer(list, a.oldData.id)) {
            puts("Cannot undo DELETE: ID was used again.");
        } else {
            addCustomer(list, a.oldData);
            enqueue(q, a.oldData);   // quay lai cuoi hang doi
            printf("Undo DELETE: restored %s (at the end of the queue).\n", a.oldData.id);
        }
    } else if (strcmp(a.type, "UPDATE") == 0) {
        updateCustomer(list, a.oldData);
        updateInQueue(q, a.oldData);
        printf("Undo UPDATE: restored old data of %s.\n", a.oldData.id);
    }
    printQueue(q);
}

void doReport(const Queue* q, const Stats* st) {
    puts("----- END-OF-DAY REPORT -----");
    printf("Served customers : %d\n", st->served);
    printf("Total waiting    : %ld min", st->totalWait);
    if (st->served > 0) printf(" (avg %.2f)", (double)st->totalWait / st->served);
    printf("\nTotal wait cost  : %ld\n", st->totalCost);
    printQueue(q);
}

/* ===================== TOI UU THU TU wj/pj ===================== */
// Khach x dung truoc y neu wx/px > wy/py  <=>  wx*py > wy*px (tranh so thuc)
int cmpRatio(const void* a, const void* b) {
    const Customer* x = (const Customer*)a;
    const Customer* y = (const Customer*)b;
    long l = (long)x->waitingWeight * y->processingTime;
    long r = (long)y->waitingWeight * x->processingTime;
    return (l < r) - (l > r);
}

long totalCostOf(const Customer* arr, int n, int startTime) {
    long t = startTime, cost = 0;
    for (int i = 0; i < n; i++) {
        t += arr[i].processingTime;
        cost += (long)arr[i].waitingWeight * t;
    }
    return cost;
}

void doOptimize(const Queue* q, const Stats* st) {
    if (q->size == 0) { puts("Queue is empty."); return; }
    Customer* arr = (Customer*)malloc(q->size * sizeof(Customer));
    if (!arr) { puts("Out of memory."); return; }

    int n = 0;
    for (Node* p = q->front; p; p = p->next) arr[n++] = p->data;

    long fifo = totalCostOf(arr, n, st->clock);   // gia su moi khach da co mat
    qsort(arr, n, sizeof(Customer), cmpRatio);     // O(n log n)
    long best = totalCostOf(arr, n, st->clock);

    printf("FIFO cost   : %ld\n", fifo);
    printf("Sorted order (w/p desc): ");
    for (int i = 0; i < n; i++)
        printf("%s(%.2f)%s", arr[i].id,
               (double)arr[i].waitingWeight / arr[i].processingTime,
               i < n - 1 ? " -> " : "\n");
    printf("Sorted cost : %ld\n", best);
    printf("Saved       : %ld\n", fifo - best);
    free(arr);
}

/* ===================== MAIN ===================== */
void printMenu(void) {
    puts("\n===== SMART SERVICE DESK =====");
    puts("1. Add customer");
    puts("2. Serve next customer");
    puts("3. Search / Update customer");
    puts("4. Undo last action");
    puts("5. Print report");
    puts("6. Delete customer from queue");
    puts("7. Compare FIFO vs w/p order");
    puts("0. Exit");
}

int main(void) {
    Queue q = {NULL, NULL, 0};
    List list = {NULL, 0};
    Stack history = {NULL, 0};
    Stats st = {0, 0, 0, 0};
    int choice;

    do {
        printMenu();
        choice = readInt("Choose: ", 0);
        switch (choice) {
            case 1: doAdd(&list, &q, &history); break;
            case 2: doServe(&q, &st); break;
            case 3: doSearchUpdate(&list, &q, &history); break;
            case 4: undo(&history, &list, &q); break;
            case 5: doReport(&q, &st); break;
            case 6: doDelete(&list, &q, &history); break;
            case 7: doOptimize(&q, &st); break;
            case 0: puts("Bye!"); break;
            default: puts("Invalid choice.");
        }
    } while (choice != 0);

    freeQueue(&q);
    freeList(&list);
    freeStack(&history);
    return 0;
}
