#include "os.h"
#include "sqlite.h"


//访问文件方法start

int osClose(SqlFile *file) {
    return file->p_methods->xClose(file); 
}

int osRead(SqlFile *file, void *buf, int buf_sz, int offset) {
    return file->p_methods->xRead(file, buf, buf_sz, offset);
}

int osWrite(SqlFile *file, const void *content, int sz, int offset) {
    return file->p_methods->xWrite(file, content, sz, offset);
}

int osTruncate(SqlFile *file, int sz) {
    return file->p_methods->xTruncate(file, sz);
}

//访问文件方法end

//访问虚拟文件系统方法start

int osOpen(SqlVFS *vfs, const char *fileName, SqlFile *ret, int flags) {
    return vfs->xOpen(vfs, fileName, ret, flags);
}

int osDelete(SqlVFS *vfs, const char *filename) {
    return vfs->xDelete(vfs, filename);
}

int osAccess(SqlVFS *vfs, const char *filename, int flag, int *ret) {
    return vfs->xAccess(vfs, filename, flag, ret);
}

int osSleep(SqlVFS *vfs, int micro_secs) {
    return vfs->xSleep(vfs, micro_secs);
}

int osCurrentTime(SqlVFS *vfs, int *ret) {
    return vfs->xCurrentTime(vfs, ret);
}

SqlFile *osGetFileHandle(SqlVFS *vfs) {
    SqlFile *p = (SqlFile *)malloc(vfs->sz_file);
    return p;
}

SqlVFS *osGetVFS(const char *vfs_name) {
    if(strcmp(vfs_name, "unix") == 0) {
        return (SqlVFS *) unixGetOS();
    }
    return 0;
}
//访问虚拟文件系统方法end

