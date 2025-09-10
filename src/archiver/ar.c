#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

void __path_R(char *path, const char *dir_to_remove) {
  size_t len = strlen(dir_to_remove);
  if (strncmp(path, dir_to_remove, len) == 0 &&
      (path[len] == '/' || path[len] == '\0')) {
    memmove(path, path + len + (path[len] == '/' ? 1 : 0),
            strlen(path) - len + 1);
  }
}

void setup_archive_support(struct archive *a, const char *filename) {
  const char *dot = strrchr(filename, '.');
  int is_pure_gz = 0;

  if (dot && (strcmp(dot, ".gz") == 0 || strcmp(dot, ".gzip") == 0)) {
    /* Если имя заканчивается на .tar.gz или .tar.gzip — это не «чистый» gzip */
    if (!(strlen(filename) >= 7 &&
          (strcmp(filename + strlen(filename) - 7, ".tar.gz") == 0 ||
           strcmp(filename + strlen(filename) - 9, ".tar.gzip") == 0))) {
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
}

void extract(const char *filename, const char *output_dir, int show_contents) {
  struct archive *a = archive_read_new();
  struct archive *ext = archive_write_disk_new();
  struct archive_entry *entry;
  int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
              ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
  int r;

  if (!a || !ext) {
    fprintf(stderr, "Failed to create archive objects\n");
    exit(1);
  }

  setup_archive_support(a, filename);

  archive_write_disk_set_options(ext, flags);
  archive_write_disk_set_standard_lookup(ext);

  if ((r = archive_read_open_filename(a, filename, 10240)) != ARCHIVE_OK) {
    fprintf(stderr, "Error opening archive '%s': %s\n", filename,
            archive_error_string(a));
    archive_read_free(a);
    archive_write_free(ext);
    exit(1);
  }

  while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
    const char *path = archive_entry_pathname(entry);
    char clean_path[1024] = {0};

    const char *dot = strrchr(filename, '.');
    int is_pure_gz = 0;
    if (dot && (strcmp(dot, ".gz") == 0 || strcmp(dot, ".gzip") == 0)) {
      if (!(strlen(filename) >= 7 &&
            (strcmp(filename + strlen(filename) - 7, ".tar.gz") == 0 ||
             strcmp(filename + strlen(filename) - 9, ".tar.gzip") == 0))) {
        is_pure_gz = 1;
      }
    }

    if (is_pure_gz) {
      const char *last_slash = strrchr(filename, '/');
      const char *base_name = last_slash ? last_slash + 1 : filename;
      strncpy(clean_path, base_name, sizeof(clean_path) - 1);
      char *gz_ext = strstr(clean_path, ".gz");
      if (gz_ext)
        *gz_ext = '\0';
      gz_ext = strstr(clean_path, ".gzip");
      if (gz_ext)
        *gz_ext = '\0';
    } else {
      if (path)
        strncpy(clean_path, path, sizeof(clean_path) - 1);
      __path_R(clean_path, "t");
    }

    char fullpath[2048] = {0};
    if (output_dir && output_dir[0] != '\0') {
      char clean_output[1024];
      strncpy(clean_output, output_dir, sizeof(clean_output) - 1);
      size_t outlen = strlen(clean_output);
      if (outlen > 0 && clean_output[outlen - 1] == '/')
        clean_output[outlen - 1] = '\0';
      snprintf(fullpath, sizeof(fullpath), "%s/%s", clean_output, clean_path);
    } else {
      strncpy(fullpath, clean_path, sizeof(fullpath) - 1);
    }

    archive_entry_set_pathname(entry, fullpath);

    r = archive_write_header(ext, entry);
    if (r < ARCHIVE_OK) {
      fprintf(stderr, "Warning writing header: %s\n",
              archive_error_string(ext));
    }

    if (archive_entry_size(entry) > 0) {
      const void *buff;
      size_t size;
      off_t offset;
      while ((r = archive_read_data_block(a, &buff, &size, &offset)) ==
             ARCHIVE_OK) {
        r = archive_write_data_block(ext, buff, size, offset);
        if (r < ARCHIVE_OK) {
          fprintf(stderr, "Error writing data block: %s\n",
                  archive_error_string(ext));
          break;
        }
      }
      if (r != ARCHIVE_EOF && r < ARCHIVE_OK) {
        fprintf(stderr, "Error reading data block: %s\n",
                archive_error_string(a));
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

  if (r != ARCHIVE_EOF) {
    fprintf(stderr, "Archive read error: %s\n", archive_error_string(a));
  }

  archive_read_close(a);
  archive_read_free(a);
  archive_write_close(ext);
  archive_write_free(ext);
}

void list(const char *filename) {
  struct archive *a = archive_read_new();
  struct archive_entry *entry;
  int r;

  if (!a) {
    fprintf(stderr, "Failed to create archive object\n");
    return;
  }

  setup_archive_support(a, filename);

  if ((r = archive_read_open_filename(a, filename, 10240)) != ARCHIVE_OK) {
    fprintf(stderr, "Error opening archive '%s': %s\n", filename,
            archive_error_string(a));
    archive_read_free(a);
    return;
  }

  while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
    const char *path = archive_entry_pathname(entry);
    time_t mtime = archive_entry_mtime(entry);
    const char *timestr = ctime(&mtime);
    char timestr_trimmed[32] = {0};

    if (timestr) {
      strncpy(timestr_trimmed, timestr, sizeof(timestr_trimmed) - 1);
      size_t tlen = strlen(timestr_trimmed);
      if (tlen > 0 && timestr_trimmed[tlen - 1] == '\n')
        timestr_trimmed[tlen - 1] = '\0';
    }

    printf("%s %10" PRIdMAX " %s %s\n", archive_entry_strmode(entry),
           (intmax_t)archive_entry_size(entry), timestr_trimmed,
           path ? path : "");

    archive_read_data_skip(a);
  }

  if (r != ARCHIVE_EOF) {
    fprintf(stderr, "Archive read error: %s\n", archive_error_string(a));
  }

  archive_read_free(a);
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "--help") == 0) {
    printf("Usage: %s [OPTION]... [FILE]...\n", argv[0]);
    printf("Extract and list archive contents.\n\n");
    printf("Options:\n");
    printf(
        "  -t, --list <archive>               List contents of the archive\n");
    printf("  -x, --extract <archive>            Extract files from the "
           "archive\n");
    printf("  -g, --unzip-check                  Show contents while "
           "extracting\n");
    printf("  -o, --output <dir>                 Specify output directory for "
           "extraction\n");
    printf("      --help                         Display this help and exit\n");
    return 0;
  }

  int show_contents = 0;
  const char *output_dir = NULL;
  const char *archive_path = NULL;
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

  if (!archive_path) {
    fprintf(stderr,
            "No archive specified. Use --help for usage information.\n");
    return 1;
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
