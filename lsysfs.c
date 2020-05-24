/**
 * Less Simple, Yet Stupid Filesystem.
 * 
 * Mohammed Q. Hussain - http://www.maastaar.net
 *
 * This is an example of using FUSE to build a simple filesystem. It is a part of a tutorial in MQH Blog with the title "Writing Less Simple, Yet Stupid Filesystem Using FUSE in C": http://maastaar.net/fuse/linux/filesystem/c/2019/09/28/writing-less-simple-yet-stupid-filesystem-using-FUSE-in-C/
 *
 * License: GNU GPL
 */
 
#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// ... //
#define LIMIT 256
struct timearray
{
  timespec time[/* File Index */ LIMIT];
};


char dir_list[ /* Dir Index */ LIMIT ][ /* Dir Name */ LIMIT ];
int curr_dir_idx = -1;

//TODO: Change to STL container for big directory support
char files_list[ /* File Index */ LIMIT ][ /* File Name */ LIMIT ];
int curr_file_idx = -1;

//TODO: Change to STL container for Large file support
char files_content[ /* File Index */ LIMIT ][ /* Content */ LIMIT ];
int curr_file_content_idx = -1;

enum TIMETYPE
{
	ATIME,
	CTIME,
	MTIME
};

// For atime/ctime/mtime time respectively
struct timearray dirs_time[3];
struct timearray files_time[3];



void add_dir( const char *dir_name )
{
	curr_dir_idx++;
	strcpy( dir_list[ curr_dir_idx ], dir_name );

	// Init dir timestamp when crating
	struct timespec now;
	timespec_get(&now, TIME_UTC);
	dirs_time[ATIME].time[curr_dir_idx] = now;
	dirs_time[CTIME].time[curr_dir_idx] = now;
	dirs_time[MTIME].time[curr_dir_idx] = now;
}

int is_dir( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

void add_file( const char *filename )
{
	curr_file_idx++;
	strcpy( files_list[ curr_file_idx ], filename );

	curr_file_content_idx++;
	strcpy( files_content[ curr_file_content_idx ], "" );

		
	// Init file timestamp when crating
	struct timespec now;
	timespec_get(&now, TIME_UTC);
	files_time[ATIME].time[curr_file_idx] = now;
	files_time[CTIME].time[curr_file_idx] = now;
	files_time[MTIME].time[curr_file_idx] = now;
}

int is_file( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

int get_file_index( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return curr_idx;
	
	return -1;
}

int get_dir_index( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx < LIMIT; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return curr_idx;
	
	return -1;
}

void write_to_file( const char *path, const char *new_content )
{
	int file_idx = get_file_index( path );
	if ( file_idx == -1 ) // No such file
		return;
		
	strcpy( files_content[ file_idx ], new_content ); 

	// The last "m"odification of the file is right now
	timespec_get(&files_time[MTIME].time[curr_file_idx], TIME_UTC);
}

int remove_file( const int file_idx){
	for ( int idx = file_idx; idx < LIMIT-1; idx++ ){
		strcpy(files_list[idx], files_list[idx+1]);
		strcpy(files_content[idx], files_content[idx+1]);
		files_time[ATIME].time[idx] = files_time[ATIME].time[idx+1];
		files_time[CTIME].time[idx] = files_time[CTIME].time[idx+1];
		files_time[MTIME].time[idx] = files_time[MTIME].time[idx+1];
	}
	curr_file_idx--;
	curr_file_content_idx--;
	return 0;
}

int remove_dir( const char* path){
	path++; // Eliminating "/" in the path
	int idx;
	for ( idx = 0; idx < LIMIT; idx++ )
		if ( strcmp( path, dir_list[ idx ] ) == 0 )
			break;
	for ( idx; idx < LIMIT-1; idx++ ){
		strcpy(dir_list[idx], dir_list[idx+1]);
		dirs_time[ATIME].time[idx] = dirs_time[ATIME].time[idx+1];
		dirs_time[CTIME].time[idx] = dirs_time[CTIME].time[idx+1];
		dirs_time[MTIME].time[idx] = dirs_time[MTIME].time[idx+1];
	}
	curr_dir_idx--;
	return 0;
}

// ... //

static int do_getattr( const char *path, struct stat *st )
{
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	// st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	// st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	// st->st_ctime = time( NULL ); // The last "c"hange of the file/directory is right now
	
	if ( strcmp( path, "/" ) == 0 || is_dir( path ) == 1 )
	{
		int idx = get_dir_index(path);
		st->st_atim = dirs_time[ATIME].time[idx];
		st->st_mtim = dirs_time[MTIME].time[idx];
		st->st_ctim = dirs_time[CTIME].time[idx];
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
	}
	else if ( is_file( path ) == 1 )
	{
		int idx = get_file_index(path);
		st->st_atim = files_time[ATIME].time[idx];
		st->st_mtim = files_time[MTIME].time[idx];
		st->st_ctim = files_time[CTIME].time[idx];

		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	else
	{
		return -ENOENT;
	}
	
	return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	filler( buffer, ".", NULL, 0 ); // Current Directory
	filler( buffer, "..", NULL, 0 ); // Parent Directory
	
	if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
	{
		for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
			filler( buffer, dir_list[ curr_idx ], NULL, 0 );
	
		for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
			filler( buffer, files_list[ curr_idx ], NULL, 0 );
	}
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 )
		return -1;
	
	// The last "a"ccess of the file is right now
	timespec_get(&files_time[ATIME].time[curr_file_idx], TIME_UTC);

	char *content = files_content[ file_idx ];
	
	memcpy( buffer, content + offset, size );
		
	return strlen( content ) - offset;
}

static int do_mkdir( const char *path, mode_t mode )
{
	path++;
	add_dir( path );
	
	return 0;
}

static int do_mknod( const char *path, mode_t mode, dev_t rdev )
{
	path++;
	add_file( path );
	return 0;
}

static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
	write_to_file( path, buffer );
	return size;
}

static int do_unlink( const char *path)
{
	if(is_file( path ))
	{
		int file_idx = get_file_index( path );
		if ( file_idx == -1 ) // No such file
			return -1;
		return remove_file(file_idx);
	}
	return -1;
}

static int do_rmdir( const char *path)
{
	if(is_dir( path ))
		return remove_dir(path);
	return -1;
}

static int do_utimens( const char *path, const struct timespec tv[2])
{
	if(is_file( path ))
	{
		int file_idx = get_file_index( path );
		if ( file_idx == -1 ) // No such file
			return -1;

		struct timespec now;
		timespec_get(&now, TIME_UTC);
		files_time[ATIME].time[file_idx] = now;
		files_time[CTIME].time[file_idx] = now;
		files_time[MTIME].time[file_idx] = now;
	}else if(is_dir( path )){
		int dir_idx = get_dir_index(path);
		if ( dir_idx == -1 ) // No such dir
			return -1;

		struct timespec now;
		timespec_get(&now, TIME_UTC);
		dirs_time[ATIME].time[dir_idx] = now;
		dirs_time[CTIME].time[dir_idx] = now;
		dirs_time[MTIME].time[dir_idx] = now;
	}
	// Only cease the error of stime
	return 0;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr,
	.mknod		= do_mknod,
	.mkdir		= do_mkdir,
	.unlink		= do_unlink,
	.rmdir		= do_rmdir,
	.read		= do_read,
    .write		= do_write,
    .readdir	= do_readdir,
	.utimens	= do_utimens,
};

int main( int argc, char *argv[] )
{
	return fuse_main( argc, argv, &operations, NULL );
}
