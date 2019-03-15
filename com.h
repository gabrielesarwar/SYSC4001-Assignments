#define ARRAY_SIZE 5

struct shared_array {
    int B[ARRAY_SIZE];
};

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
