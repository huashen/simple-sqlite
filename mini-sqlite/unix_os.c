#include "sqlite.h"

//unix文件打开方式
#define SQL_DEFAULT_FILE_CREATE_MODE 0744

typedef struct UnixFile UnixFile;

struct UnixFile
{
    struct SqlFile base; //基础数据 必须在第一行

    int fd; //文件描述符
    char *filename;//文件名称
    UnixFile *next;//下一unix文件
} *head = 0; //指向vfs中第一个打开的文件

/**
   检查此文件是否已打开
   如果打开则返回其指针
   否则为 NULL
 * 
 */
UnixFile *unixAlreadyOpened(const char *filename) {
    UnixFile *p = head;
    for (;p && strcmp(p->filename, filename); p=p->next) {
        return p;
    }
}

/**
 * @brief 打开文件
 * 
 * @param file 
 * @param filename 
 * @param flags 
 * @return int 
 */
int unixOpenFile(UnixFile *file, const char *filename, int flags) {
    int fd = open(filename, flags, SQL_DEFAULT_FILE_CREATE_MODE);
    
    if(fd < 0) {
        return SQL_ERROR;
    }

    file->fd = fd;
    int n = strlen(filename);
    file->filename = (char *)malloc(n + 1);
    memcpy(file->filename, filename, n);
    file->filename[n] = 0;

    return SQL_OK;
}

//unix下文件操作具体方法实现start

/**
 * @brief 关闭文件
 * 
 * @param file 
 * @return int 
 */
int unixClose(SqlFile *file) {
    if(!file) return SQL_ERROR;

    UnixFile *dp = (UnixFile *)file;

    UnixFile **p = &head;
    for(;*p != dp && *p; p = &((*p) -> next)) {}
    if(*p == 0) return SQL_ERROR;
    *p = dp->next;

    int ret = close(dp->fd);
    free(dp->filename);

    return ret;
}

int unixRead(SqlFile *file, void *buf, int buf_sz, int offset) {
    if(!file || !buf) return SQL_ERROR;

    UnixFile *p = (UnixFile *) file;
    int off = lseek(p->fd, offset, SEEK_SET);
    if(off != offset) return SQL_ERROR;

    return read(p->fd, buf, buf_sz);
}

int unixWrite(SqlFile *file, const void *content, int sz, int offset) {
    if(!file) return SQL_ERROR; 

    UnixFile *p = (UnixFile *)file;
    int off = lseek(p->fd, offset, SEEK_SET);
    
    if (off != offset) return SQL_ERROR;

    return write(p->fd, content, sz);
}

int unixTruncate(SqlFile *file, int sz) {
    if(!file)  return SQL_ERROR;

    UnixFile *p = (UnixFile *) file;
    return ftruncate(p->fd, sz);
}

int unixSync(SqlFile *file) {
    if(!file)  return SQL_ERROR;

    UnixFile *p = (UnixFile *) file;
    return syncfs(p->fd);
}

int unixFileSize(SqlFile *file, int *ret) {
    if(!file || !ret)  return SQL_ERROR;

    UnixFile *p = (UnixFile *) file;
    struct stat st;
    if(fstat(p->fd, &st) < 0) return SQL_ERROR;

    *ret = st.st_size;
    return SQL_OK;
}

//unix下文件操作具体方法实现end


//访问虚拟文件系统的方法


int unixOpen(SqlVFS *vfs, const char *filename, SqlFile *ret, int flags) {
    const static SqlIOMethods UNIX_METHODS = {
        unixClose,
        unixRead,
        unixWrite,
        unixTruncate,
        unixSync,
        unixFileSize
    }

    if(!ret || !vfs || !filename) return SQL_ERROR;

    UnixFile *dp = unixAlreadyOpened(filename);
    if(dp) return SQL_ERROR;

    UnixFile *p = (UnixFile *) ret;
    p->base.p_methods = &UNIX_METHODS;

    p->next = head;
    head = p;
    return SQL_OK;
}

int unixDelete(SqlFile *vfs, const char *filename) {
    if(!filename) return SQL_ERROR;

    UnixFile *dp = unixAlreadyOpened(filename);

    if (dp) unixClose((SqlFile *) dp);

    return unlink(filename);
}

int unixAccess(SqlVFS *vfs, const char *filename, int flag, int *ret) {
    if(!vfs || !filename || !ret) return SQL_ERROR;

    *ret = access(filename, flag);
    return SQL_OK;
}

int unixSleep(SqlVFS *vfs, int micro_secs) {
    static const int T = 1000000;
    sleep(micro_secs / T);
    usleep(micro_secs % T);
    return SQL_OK;
}

int unixCurrentTime(SqlVFS *vfs, int *ret) {
    time(ret);
    return SQL_OK;
}


SqlVFS *unixGetOS() {
    static struct SqlVFS UNIX_OS = {
        sizeof(UnixFile),
        200,
        "unix",
        0,

        unixOpen,
        unixDelete,
        unixAccess,
        unixSleep,
        unixCurrentTime
    };
    return &UNIX_OS;
}
