/*
   archiver/ar.c
   https:
    
*/

#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void __path_R(char* path, const char* dir_to_remove) {
  size_t len = strlen(dir_to_remove);
  if (strncmp(path, dir_to_remove, len) == 0 &&
      (path[len] == '/' || path[len] == '\0')) {
    memmove(path, path + len + (path[len] == '/' ? 1 : 0),
            strlen(path) - len + 1);
  }
}

void extract(const char* filename, const char* output_dir, int show_contents) {
  struct archive* a = archive_read_new();
  struct archive* ext = archive_write_disk_new();
  struct archive_entry* entry;
  int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
              ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
  int r;

  
  int is_pure_gz = 0;
  const char *dot = strrchr(filename, '.');
  if (dot && (strcmp(dot, ".gz") == 0 || strcmp(dot, ".gzip") == 0)) {
    const char *prev_dot = strrchr(filename, '.');
    if (prev_dot == dot) {  
      is_pure_gz = 1;
    }
  }

  if (is_pure_gz) {
    
    archive_read_support_format_raw(a);
    archive_read_support_filter_gzip(a);
  } else {
    
    archive_read_support_format_7zip(a);     
    archive_read_support_format_ar(a);       
    archive_read_support_format_cab(a);      
    archive_read_support_format_cpio(a);     
    archive_read_support_format_gnutar(a);   
    archive_read_support_format_iso9660(a);  
    archive_read_support_format_lha(a);      
    archive_read_support_format_rar(a);      
    archive_read_support_format_rar5(a);    
    archive_read_support_format_tar(a);     
    archive_read_support_format_warc(a);    
    archive_read_support_format_xar(a);     
    archive_read_support_format_zip(a);     

    archive_read_support_filter_bzip2(a);     
    archive_read_support_filter_compress(a);  
    archive_read_support_filter_gzip(a);      
    archive_read_support_filter_lz4(a);       
    archive_read_support_filter_lzma(a);      
    archive_read_support_filter_lzip(a);      
    archive_read_support_filter_xz(a);        
    archive_read_support_filter_zstd(a);      
  }

  archive_write_disk_set_options(ext, flags);
  archive_write_disk_set_standard_lookup(ext);

  if ((r = archive_read_open_filename(a, filename, 10240))) {
    fprintf(stderr, "Error opening archive: %s\n", archive_error_string(a));
    exit(1);
  }

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    const char* path = archive_entry_pathname(entry);
    char clean_path[1024];
    
    if (is_pure_gz) {
      
      const char *last_slash = strrchr(filename, '/');
      const char *base_name = last_slash ? last_slash + 1 : filename;
      strncpy(clean_path, base_name, sizeof(clean_path));
      
      
      char *gz_ext = strstr(clean_path, ".gz");
      if (gz_ext) *gz_ext = '\0';
      gz_ext = strstr(clean_path, ".gzip");
      if (gz_ext) *gz_ext = '\0';
    } else {
      strncpy(clean_path, path, sizeof(clean_path));
      __path_R(clean_path, "t");
    }

    char fullpath[1024];
    if (output_dir) {
      char clean_output[1024];
      strncpy(clean_output, output_dir, sizeof(clean_output));
      if (clean_output[strlen(clean_output) - 1] == '/') {
        clean_output[strlen(clean_output) - 1] = '\0';
      }

      snprintf(fullpath, sizeof(fullpath), "%s/%s", clean_output, clean_path);
    } else {
      strncpy(fullpath, clean_path, sizeof(fullpath));
    }

    archive_entry_set_pathname(entry, fullpath);

    r = archive_write_header(ext, entry);
    if (r < ARCHIVE_OK) {
      fprintf(stderr, "Error writing header: %s\n", archive_error_string(ext));
    }

    if (archive_entry_size(entry) > 0) {
      const void* buff;
      size_t size;
      off_t offset;

      while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
        archive_write_data_block(ext, buff, size, offset);
      }
    }

    r = archive_write_finish_entry(ext);
    if (r < ARCHIVE_OK) {
      fprintf(stderr, "Error finishing entry: %s\n", archive_error_string(ext));
    }

    if (show_contents) {
      printf("%s\n", fullpath);
    }
  }

  archive_read_close(a);
  archive_read_free(a);
  archive_write_close(ext);
  archive_write_free(ext);
}

void list(const char* filename) {
  struct archive* a;
  struct archive_entry* entry;
  int r;

  a = archive_read_new();

  
  int is_pure_gz = 0;
  const char *dot = strrchr(filename, '.');
  if (dot && (strcmp(dot, ".gz") == 0 || strcmp(dot, ".gzip") == 0)) {
    const char *prev_dot = strrchr(filename, '.');
    if (prev_dot == dot) {  
      is_pure_gz = 1;
    }
  }

  if (is_pure_gz) {
    archive_read_support_format_raw(a);
    archive_read_support_filter_gzip(a);
  } else {
    archive_read_support_format_7zip(a);
    archive_read_support_format_ar(a);
    archive_read_support_format_cab(a);
    archive_read_support_format_cpio(a);
    archive_read_support_format_gnutar(a);
    archive_read_support_format_iso9660(a);
    archive_read_support_format_lha(a);
    archive_read_support_format_rar(a);
    archive_read_support_format_rar5(a);
    archive_read_support_format_raw(a);
    archive_read_support_format_tar(a);
    archive_read_support_format_warc(a);
    archive_read_support_format_xar(a);
    archive_read_support_format_zip(a);

    archive_read_support_filter_bzip2(a);
    archive_read_support_filter_compress(a);
    archive_read_support_filter_gzip(a);
    archive_read_support_filter_lz4(a);
    archive_read_support_filter_lzma(a);
    archive_read_support_filter_lzip(a);
    archive_read_support_filter_xz(a);
    archive_read_support_filter_zstd(a);
  }

  if ((r = archive_read_open_filename(a, filename, 10240))) {
    fprintf(stderr, "Error opening archive: %s\n", archive_error_string(a));
    exit(1);
  }

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    const char* path = archive_entry_pathname(entry);
    time_t mtime = archive_entry_mtime(entry);
    const char* timestr = ctime(&mtime);
    char timestr_trimmed[32];

    strncpy(timestr_trimmed, timestr, sizeof(timestr_trimmed));
    timestr_trimmed[strlen(timestr_trimmed) - 1] = '\0';

    printf("%s %10jd %s %s\n", archive_entry_strmode(entry),
           (intmax_t)archive_entry_size(entry), timestr_trimmed, path);

    archive_read_data_skip(a);
  }

  archive_read_free(a);
}

int main(int argc, char** argv) {
  if (argc > 1 && strcmp(argv[1], "--help") == 0) {
    printf("Usage: %s [OPTION]... [FILE]...\n", argv[0]);
    printf("Extract and list archive contents.\n\n");
    printf("Options:\n");
    printf("  -t, --list <archive>               List contents of the archive\n");
    printf("  -x, --extract <archive>            Extract files from the archive\n");
    printf("  -g, --unzip-check                  Show contents while extracting\n");
    printf("  -o, --output <dir>                 Specify output directory for extraction\n");
    printf("      --help                         Display this help and exit\n");
    return 0;
  }

  int show_contents = 0;
  const char* output_dir = NULL;
  const char* archive_path = NULL;
  int mode = 0;

  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--list") == 0) &&
        i + 1 < argc) {
      mode = 1;
      archive_path = argv[++i];
    } else if ((strcmp(argv[i], "-x") == 0 ||
                strcmp(argv[i], "--extract") == 0) &&
               i + 1 < argc) {
      mode = 2;
      archive_path = argv[++i];
    } else if (strcmp(argv[i], "-g") == 0 ||
               strcmp(argv[i], "--unzip-check") == 0) {
      show_contents = 1;
    } else if ((strcmp(argv[i], "-o") == 0 ||
                strcmp(argv[i], "--output") == 0) &&
               i + 1 < argc) {
      output_dir = argv[++i];
    }
  }

  if (mode == 1) {
    list(archive_path);
  } else if (mode == 2) {
    extract(archive_path, output_dir, show_contents);
  } else {
    fprintf(stderr,
            "Invalid mode specified. Use --help for usage information.\n");
    return 1;
  }

  return 0;
}
