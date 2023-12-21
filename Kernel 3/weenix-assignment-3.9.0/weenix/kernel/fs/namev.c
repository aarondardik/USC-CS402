/******************************************************************************/
/* Important Fall 2023 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "kernel.h"
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        // NOT_YET_IMPLEMENTED("VFS: lookup");
        KASSERT(NULL != dir);
        KASSERT(NULL != name);
        KASSERT(NULL != result);
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");

        // if provided arg for dir is not a dir, return -ENOTDIR
        if (!S_ISDIR(dir->vn_mode))
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTDIR;
        }

        if (len > STR_MAX)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENAMETOOLONG;
        }

        // if len is 0, return -EINVAL
        if (len == 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }

        // if dir is a proper dir, we can call its lookup function and return what it returns
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return dir->vn_ops->lookup(dir, name, len, result);
}

/*
 * helped method for 'dir_namev' method, to enhance readability
 * */
int dir_namev_helper1(const char *pathname, size_t *namelen, const char **name,
                      vnode_t *base, vnode_t **res_vnode,
                      int beg, int end, int seg_len)
{
        vnode_t *dir_vnode = NULL;

        while (1)
        {
                while (pathname[end] != '/')
                {
                        if (pathname[end] == '\0')
                        {
                                if (seg_len > NAME_LEN)
                                {
                                        vput(base);
                                        dbg(DBG_PRINT, "(GRADING2B)\n");
                                        return -ENAMETOOLONG;
                                }
                                *res_vnode = vget(base->vn_fs, base->vn_vno);
                                vput(*res_vnode);
                                if (!S_ISDIR((*res_vnode)->vn_mode))
                                {
                                        vput(*res_vnode);
                                        dbg(DBG_PRINT, "(GRADING2B)\n");
                                        return -ENOTDIR;
                                }
                                *name = &pathname[beg];
                                *namelen = seg_len;
                                dbg(DBG_PRINT, "(GRADING2B)\n");
                                return 0;
                        }
                        end++;
                        seg_len = end - beg;
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                while (pathname[end + 1] == '/')
                {
                        end++;
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                if (pathname[end + 1] == '\0')
                {
                        end++;
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        continue;
                }
                int lookup_ret = lookup(base, &pathname[beg], seg_len, &dir_vnode);
                vput(base);
                if (lookup_ret != 0)
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        return lookup_ret;
                }
                KASSERT(NULL != dir_vnode);
                dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
                base = dir_vnode;

                beg = end + 1;
                end++;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");
        return 0;
}

/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int dir_namev(const char *pathname, size_t *namelen, const char **name,
              vnode_t *base, vnode_t **res_vnode)
{

        KASSERT(NULL != pathname);
        KASSERT(NULL != namelen);
        KASSERT(NULL != name);
        KASSERT(NULL != res_vnode);
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

        if (base == NULL)
        {
                base = curproc->p_cwd;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        if (pathname[0] == '/')
        {
                base = vfs_root_vn;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        vref(base);

        int beg = 0;
        int end = 0;
        int seg_len = 0;

        if (pathname[beg] == '/')
        {
                beg++;
                end++;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        // dbg(DBG_PRINT, "(GRADING2A) Exiting dir_namev method\n");
        return dir_namev_helper1(pathname, namelen, name, base, res_vnode, beg, end, seg_len);
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified and the file does
 * not exist, call create() in the parent directory vnode. However, if the
 * parent directory itself does not exist, this function should fail - in all
 * cases, no files or directories other than the one at the very end of the path
 * should be created.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        // NOT_YET_IMPLEMENTED("VFS: open_namev");

        size_t namelen = 0;
        const char *name = NULL;
        vnode_t *res_vnode_dir = NULL;

        int dirname_return_val = dir_namev(pathname, &namelen, &name, base, &res_vnode_dir);
        dbg(DBG_PRINT, "(GRADING2A)\n");

        if (dirname_return_val != 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return dirname_return_val;
        }
        if (strncmp(name, "", namelen) == 0)
        {
                *res_vnode = res_vnode_dir;
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return 0;
        }

        int lookup_result = lookup(res_vnode_dir, name, namelen, res_vnode);
        dbg(DBG_PRINT, "(GRADING2A)\n");

        if (lookup_result != 0)
        {
                if (flag & O_CREAT && lookup_result == -ENOENT)
                {
                        KASSERT((flag & O_CREAT) && NULL != res_vnode_dir->vn_ops->create);
                        dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
                        dbg(DBG_PRINT, "(GRADING2B)\n");

                        lookup_result = res_vnode_dir->vn_ops->create(res_vnode_dir, name, namelen, res_vnode);
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                else
                {
                        vput(res_vnode_dir);
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        // dbg(DBG_ERROR, "look_up failed. Result is: %d\n", lookup_result);
                        return lookup_result;
                }
        }

        dbg(DBG_PRINT, "(GRADING2A)\n");
        vput(res_vnode_dir);
        // dbg(DBG_ERROR, "look_up succeeded. Result is: %d\n", lookup_result);
        return lookup_result;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}

/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
