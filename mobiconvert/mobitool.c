/** @file mobitool.c
 *
 * @brief mobitool
 *
 * @example mobitool.c
 * Program for testing libmobi library
 *
 * Copyright (c) 2020 Bartek Fabiszewski
 * http://www.fabiszewski.net
 *
 * Licensed under LGPL, either version 3, or any later.
 * See <http://www.gnu.org/licenses/>
 */

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
 /* include libmobi header */
#include <mobi.h>
#include <windows.h>
#include "common.h"

/* miniz file is needed for EPUB creation */
#ifdef USE_XMLWRITER
# define MINIZ_HEADER_FILE_ONLY
# define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
# include "../src/miniz.c"
#endif

#ifdef HAVE_SYS_RESOURCE_H
/* rusage */
# include <sys/resource.h>
# define PRINT_RUSAGE_ARG "u"
#else
# define PRINT_RUSAGE_ARG ""
#endif
#include <read.h>
/* encryption */
#ifdef USE_ENCRYPTION
# define PRINT_ENC_USG " [-p pid] [-P serial]"
# define PRINT_ENC_ARG "p:P:"
#else
# define PRINT_ENC_USG ""
# define PRINT_ENC_ARG ""
#endif
/* xmlwriter */
#ifdef USE_XMLWRITER
# define PRINT_EPUB_ARG "e"
#else
# define PRINT_EPUB_ARG ""
#endif

#if HAVE_ATTRIBUTE_NORETURN 
static void exit_with_usage(const char* progname) __attribute__((noreturn));
#else
static void exit_with_usage(const char* progname);
#endif

#define EPUB_CONTAINER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n\
  <rootfiles>\n\
    <rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/>\n\
  </rootfiles>\n\
</container>"
#define EPUB_MIMETYPE "application/epub+zip"

/* command line options */
bool dump_cover_opt = false;
bool dump_rawml_opt = false;
bool create_epub_opt = false;
bool print_extended_meta_opt = false;
bool print_rec_meta_opt = false;
bool dump_rec_opt = false;
bool parse_kf7_opt = false;
bool dump_parts_opt = false;
bool print_rusage_opt = false;
bool extract_source_opt = false;
bool split_opt = false;
#ifdef USE_ENCRYPTION
bool setpid_opt = false;
bool setserial_opt = false;
#endif

/* options values */
#ifdef USE_ENCRYPTION
char* pid = NULL;
char* serial = NULL;
#endif

/**
 @brief Print all loaded headers meta information
 @param[in] m MOBIData structure
 */
static void print_meta(const MOBIData* m) {
	/* Full name stored at offset given in MOBI header */
	if (m->mh && m->mh->full_name) {
		char full_name[FULLNAME_MAX + 1];
		if (mobi_get_fullname(m, full_name, FULLNAME_MAX) == MOBI_SUCCESS) {
			printf("\nFull name: %s\n", full_name);
		}
	}
	/* Palm database header */
	if (m->ph) {
		printf("\nPalm doc header:\n");
		printf("name: %s\n", m->ph->name);
		printf("attributes: %hu\n", m->ph->attributes);
		printf("version: %hu\n", m->ph->version);
		struct tm* timeinfo = mobi_pdbtime_to_time(m->ph->ctime);
		printf("ctime: %s", asctime(timeinfo));
		timeinfo = mobi_pdbtime_to_time(m->ph->mtime);
		printf("mtime: %s", asctime(timeinfo));
		timeinfo = mobi_pdbtime_to_time(m->ph->btime);
		printf("btime: %s", asctime(timeinfo));
		printf("mod_num: %u\n", m->ph->mod_num);
		printf("appinfo_offset: %u\n", m->ph->appinfo_offset);
		printf("sortinfo_offset: %u\n", m->ph->sortinfo_offset);
		printf("type: %s\n", m->ph->type);
		printf("creator: %s\n", m->ph->creator);
		printf("uid: %u\n", m->ph->uid);
		printf("next_rec: %u\n", m->ph->next_rec);
		printf("rec_count: %u\n", m->ph->rec_count);
	}
	/* Record 0 header */
	if (m->rh) {
		printf("\nRecord 0 header:\n");
		printf("compression type: %u\n", m->rh->compression_type);
		printf("text length: %u\n", m->rh->text_length);
		printf("text record count: %u\n", m->rh->text_record_count);
		printf("text record size: %u\n", m->rh->text_record_size);
		printf("encryption type: %u\n", m->rh->encryption_type);
		printf("unknown: %u\n", m->rh->unknown1);
	}
	/* Mobi header */
	if (m->mh) {
		printf("\nMOBI header:\n");
		printf("identifier: %s\n", m->mh->mobi_magic);
		if (m->mh->header_length) { printf("header length: %u\n", *m->mh->header_length); }
		if (m->mh->mobi_type) { printf("mobi type: %u\n", *m->mh->mobi_type); }
		if (m->mh->text_encoding) { printf("text encoding: %u\n", *m->mh->text_encoding); }
		if (m->mh->uid) { printf("unique id: %u\n", *m->mh->uid); }
		if (m->mh->version) { printf("file version: %u\n", *m->mh->version); }
		if (m->mh->orth_index) { printf("orth index: %u\n", *m->mh->orth_index); }
		if (m->mh->infl_index) { printf("infl index: %u\n", *m->mh->infl_index); }
		if (m->mh->names_index) { printf("names index: %u\n", *m->mh->names_index); }
		if (m->mh->keys_index) { printf("keys index: %u\n", *m->mh->keys_index); }
		if (m->mh->extra0_index) { printf("extra0 index: %u\n", *m->mh->extra0_index); }
		if (m->mh->extra1_index) { printf("extra1 index: %u\n", *m->mh->extra1_index); }
		if (m->mh->extra2_index) { printf("extra2 index: %u\n", *m->mh->extra2_index); }
		if (m->mh->extra3_index) { printf("extra3 index: %u\n", *m->mh->extra3_index); }
		if (m->mh->extra4_index) { printf("extra4 index: %u\n", *m->mh->extra4_index); }
		if (m->mh->extra5_index) { printf("extra5 index: %u\n", *m->mh->extra5_index); }
		if (m->mh->non_text_index) { printf("non text index: %u\n", *m->mh->non_text_index); }
		if (m->mh->full_name_offset) { printf("full name offset: %u\n", *m->mh->full_name_offset); }
		if (m->mh->full_name_length) { printf("full name length: %u\n", *m->mh->full_name_length); }
		if (m->mh->locale) {
			const char* locale_string = mobi_get_locale_string(*m->mh->locale);
			if (locale_string) {
				printf("locale: %s (%u)\n", locale_string, *m->mh->locale);
			}
			else {
				printf("locale: unknown (%u)\n", *m->mh->locale);
			}
		}
		if (m->mh->dict_input_lang) {
			const char* locale_string = mobi_get_locale_string(*m->mh->dict_input_lang);
			if (locale_string) {
				printf("dict input lang: %s (%u)\n", locale_string, *m->mh->dict_input_lang);
			}
			else {
				printf("dict input lang: unknown (%u)\n", *m->mh->dict_input_lang);
			}
		}
		if (m->mh->dict_output_lang) {
			const char* locale_string = mobi_get_locale_string(*m->mh->dict_output_lang);
			if (locale_string) {
				printf("dict output lang: %s (%u)\n", locale_string, *m->mh->dict_output_lang);
			}
			else {
				printf("dict output lang: unknown (%u)\n", *m->mh->dict_output_lang);
			}
		}
		if (m->mh->min_version) { printf("minimal version: %u\n", *m->mh->min_version); }
		if (m->mh->image_index) { printf("first image index: %u\n", *m->mh->image_index); }
		if (m->mh->huff_rec_index) { printf("huffman record offset: %u\n", *m->mh->huff_rec_index); }
		if (m->mh->huff_rec_count) { printf("huffman records count: %u\n", *m->mh->huff_rec_count); }
		if (m->mh->datp_rec_index) { printf("DATP record offset: %u\n", *m->mh->datp_rec_index); }
		if (m->mh->datp_rec_count) { printf("DATP records count: %u\n", *m->mh->datp_rec_count); }
		if (m->mh->exth_flags) { printf("EXTH flags: %u\n", *m->mh->exth_flags); }
		if (m->mh->unknown6) { printf("unknown: %u\n", *m->mh->unknown6); }
		if (m->mh->drm_offset) { printf("drm offset: %u\n", *m->mh->drm_offset); }
		if (m->mh->drm_count) { printf("drm count: %u\n", *m->mh->drm_count); }
		if (m->mh->drm_size) { printf("drm size: %u\n", *m->mh->drm_size); }
		if (m->mh->drm_flags) { printf("drm flags: %u\n", *m->mh->drm_flags); }
		if (m->mh->first_text_index) { printf("first text index: %u\n", *m->mh->first_text_index); }
		if (m->mh->last_text_index) { printf("last text index: %u\n", *m->mh->last_text_index); }
		if (m->mh->fdst_index) { printf("FDST offset: %u\n", *m->mh->fdst_index); }
		if (m->mh->fdst_section_count) { printf("FDST count: %u\n", *m->mh->fdst_section_count); }
		if (m->mh->fcis_index) { printf("FCIS index: %u\n", *m->mh->fcis_index); }
		if (m->mh->fcis_count) { printf("FCIS count: %u\n", *m->mh->fcis_count); }
		if (m->mh->flis_index) { printf("FLIS index: %u\n", *m->mh->flis_index); }
		if (m->mh->flis_count) { printf("FLIS count: %u\n", *m->mh->flis_count); }
		if (m->mh->unknown10) { printf("unknown: %u\n", *m->mh->unknown10); }
		if (m->mh->unknown11) { printf("unknown: %u\n", *m->mh->unknown11); }
		if (m->mh->srcs_index) { printf("SRCS index: %u\n", *m->mh->srcs_index); }
		if (m->mh->srcs_count) { printf("SRCS count: %u\n", *m->mh->srcs_count); }
		if (m->mh->unknown12) { printf("unknown: %u\n", *m->mh->unknown12); }
		if (m->mh->unknown13) { printf("unknown: %u\n", *m->mh->unknown13); }
		if (m->mh->extra_flags) { printf("extra record flags: %u\n", *m->mh->extra_flags); }
		if (m->mh->ncx_index) { printf("NCX offset: %u\n", *m->mh->ncx_index); }
		if (m->mh->unknown14) { printf("unknown: %u\n", *m->mh->unknown14); }
		if (m->mh->unknown15) { printf("unknown: %u\n", *m->mh->unknown15); }
		if (m->mh->fragment_index) { printf("fragment index: %u\n", *m->mh->fragment_index); }
		if (m->mh->skeleton_index) { printf("skeleton index: %u\n", *m->mh->skeleton_index); }
		if (m->mh->datp_index) { printf("DATP index: %u\n", *m->mh->datp_index); }
		if (m->mh->unknown16) { printf("unknown: %u\n", *m->mh->unknown16); }
		if (m->mh->guide_index) { printf("guide index: %u\n", *m->mh->guide_index); }
		if (m->mh->unknown17) { printf("unknown: %u\n", *m->mh->unknown17); }
		if (m->mh->unknown18) { printf("unknown: %u\n", *m->mh->unknown18); }
		if (m->mh->unknown19) { printf("unknown: %u\n", *m->mh->unknown19); }
		if (m->mh->unknown20) { printf("unknown: %u\n", *m->mh->unknown20); }
	}
}

/**
 @brief Print meta data of each document record
 @param[in] m MOBIData structure
 */
static void print_records_meta(const MOBIData* m) {
	/* Linked list of MOBIPdbRecord structures holds records data and metadata */
	const MOBIPdbRecord* currec = m->rec;
	while (currec != NULL) {
		printf("offset: %u\n", currec->offset);
		printf("size: %zu\n", currec->size);
		printf("attributes: %hhu\n", currec->attributes);
		printf("uid: %u\n", currec->uid);
		printf("\n");
		currec = currec->next;
	}
}

/**
 @brief Create new path. Name is derived from input file path.
		[dirname]/[basename][suffix]
 @param[out] newpath Created path
 @param[in] buf_len Created path buffer size
 @param[in] fullpath Input file path
 @param[in] suffix Path name suffix
 @return SUCCESS or ERROR
 */
static int create_path(char* newpath, const size_t buf_len, const char* fullpath, const char* suffix) {
	char dirname[FILENAME_MAX];
	char basename[FILENAME_MAX];
	split_fullpath(fullpath, dirname, basename, FILENAME_MAX);
	int n;
	if (outdir_opt) {
		n = snprintf(newpath, buf_len, "%s%s%s", outdir, basename, suffix);
	}
	else {
		n = snprintf(newpath, buf_len, "%s%s%s", dirname, basename, suffix);
	}
	if (n < 0) {
		printf("Creating file name failed\n");
		return ERROR;
	}
	if ((size_t)n >= buf_len) {
		printf("File name too long\n");
		return ERROR;
	}
	return SUCCESS;
}

/**
 @brief Create directory. Path is derived from input file path.
		[dirname]/[basename][suffix]
 @param[out] newdir Created directory path
 @param[in] buf_len Created directory buffer size
 @param[in] fullpath Input file path
 @param[in] suffix Directory name suffix
 @return SUCCESS or ERROR
 */
static int create_dir(char* newdir, const size_t buf_len, const char* fullpath, const char* suffix) {
	if (create_path(newdir, buf_len, fullpath, suffix) == ERROR) {
		return ERROR;
	}
	return make_directory(newdir);
}

/**
 @brief Dump each document record to a file into created folder
 @param[in] m MOBIData structure
 @param[in] fullpath File path will be parsed to build basenames of dumped records
 @return SUCCESS or ERROR
 */
static int dump_records(const MOBIData* m, const char* fullpath) {
	char newdir[FILENAME_MAX];
	if (create_dir(newdir, sizeof(newdir), fullpath, "_records") == ERROR) {
		return ERROR;
	}
	printf("Saving records to %s\n", newdir);
	/* Linked list of MOBIPdbRecord structures holds records data and metadata */
	const MOBIPdbRecord* currec = m->rec;
	int i = 0;
	while (currec != NULL) {
		char name[FILENAME_MAX];
		snprintf(name, sizeof(name), "record_%i_uid_%i", i++, currec->uid);
		if (write_to_dir(newdir, name, currec->data, currec->size) == ERROR) {
			return ERROR;
		}

		currec = currec->next;
	}
	return SUCCESS;
}

/**
 @brief Dump all text records, decompressed and concatenated, to a single rawml file
 @param[in] m MOBIData structure
 @param[in] fullpath File path will be parsed to create a new name for saved file
 @return SUCCESS or ERROR
 */
static int dump_rawml(const MOBIData* m, const char* fullpath) {
	char newpath[FILENAME_MAX];
	if (create_path(newpath, sizeof(newpath), fullpath, ".rawml") == ERROR) {
		return ERROR;
	}
	printf("Saving rawml to %s\n", newpath);
	errno = 0;
	FILE* file = fopen(newpath, "wb");
	if (file == NULL) {
		int errsv = errno;
		printf("Could not open file for writing: %s (%s)\n", newpath, strerror(errsv));
		return ERROR;
	}
	const MOBI_RET mobi_ret = mobi_dump_rawml(m, file);
	fclose(file);
	if (mobi_ret != MOBI_SUCCESS) {
		printf("Dumping rawml file failed (%s)\n", libmobi_msg(mobi_ret));
		return ERROR;
	}
	return SUCCESS;
}

/**
 @brief Dump cover record
 @param[in] m MOBIData structure
 @param[in] fullpath File path will be parsed to create a new name for saved file
 @return SUCCESS or ERROR
 */
static int dump_cover(const MOBIData* m, const char* fullpath) {

	MOBIPdbRecord* record = NULL;
	MOBIExthHeader* exth = mobi_get_exthrecord_by_tag(m, EXTH_COVEROFFSET);
	if (exth) {
		uint32_t offset = mobi_decode_exthvalue(exth->data, exth->size);
		size_t first_resource = mobi_get_first_resource_record(m);
		size_t uid = first_resource + offset;
		record = mobi_get_record_by_seqnumber(m, uid);
	}
	if (record == NULL || record->size < 4) {
		printf("Cover not found\n");
		return ERROR;
	}

	const unsigned char jpg_magic[] = "\xff\xd8\xff";
	const unsigned char gif_magic[] = "\x47\x49\x46\x38";
	const unsigned char png_magic[] = "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a";
	const unsigned char bmp_magic[] = "\x42\x4d";

	char ext[4] = "raw";
	if (memcmp(record->data, jpg_magic, 3) == 0) {
		snprintf(ext, sizeof(ext), "%s", "jpg");
	}
	else if (memcmp(record->data, gif_magic, 4) == 0) {
		snprintf(ext, sizeof(ext), "%s", "gif");
	}
	else if (record->size >= 8 && memcmp(record->data, png_magic, 8) == 0) {
		snprintf(ext, sizeof(ext), "%s", "png");
	}
	else if (record->size >= 6 && memcmp(record->data, bmp_magic, 2) == 0) {
		const size_t bmp_size = (uint32_t)record->data[2] | ((uint32_t)record->data[3] << 8) |
			((uint32_t)record->data[4] << 16) | ((uint32_t)record->data[5] << 24);
		if (record->size == bmp_size) {
			snprintf(ext, sizeof(ext), "%s", "bmp");
		}
	}

	char suffix[12];
	snprintf(suffix, sizeof(suffix), "_cover.%s", ext);

	char cover_path[FILENAME_MAX];
	if (create_path(cover_path, sizeof(cover_path), fullpath, suffix) == ERROR) {
		return ERROR;
	}

	printf("Saving cover to %s\n", cover_path);

	return write_file(record->data, record->size, cover_path);
}

/**
 @brief Dump parsed markup files and resources into created folder
 @param[in] rawml MOBIRawml structure holding parsed records
 @param[in] fullpath File path will be parsed to build basenames of dumped records
 @return SUCCESS or ERROR
 */
static int dump_rawml_parts(const MOBIRawml* rawml, const char* fullpath) {
	if (rawml == NULL) {
		printf("Rawml structure not initialized\n");
		return ERROR;
	}

	char newdir[FILENAME_MAX];
	if (create_dir(newdir, sizeof(newdir), fullpath, "_markup") == ERROR) {
		return ERROR;
	}
	printf("Saving markup to %s\n", newdir);

	if (create_epub_opt) {
		/* create META_INF directory */
		char opfdir[FILENAME_MAX];
		if (create_subdir(opfdir, sizeof(opfdir), newdir, "META-INF") == ERROR) {
			return ERROR;
		}

		/* create container.xml */
		if (write_to_dir(opfdir, "container.xml", (const unsigned char*)EPUB_CONTAINER, sizeof(EPUB_CONTAINER) - 1) == ERROR) {
			return ERROR;
		}

		/* create mimetype file */
		if (write_to_dir(opfdir, "mimetype", (const unsigned char*)EPUB_MIMETYPE, sizeof(EPUB_MIMETYPE) - 1) == ERROR) {
			return ERROR;
		}

		/* create OEBPS directory */
		if (create_subdir(opfdir, sizeof(opfdir), newdir, "OEBPS") == ERROR) {
			return ERROR;
		}

		/* output everything else to OEBPS dir */
		strcpy(newdir, opfdir);
	}
	char partname[FILENAME_MAX];
	if (rawml->markup != NULL) {
		/* Linked list of MOBIPart structures in rawml->markup holds main text files */
		MOBIPart* curr = rawml->markup;
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
			snprintf(partname, sizeof(partname), "part%05zu.%s", curr->uid, file_meta.extension);
			if (write_to_dir(newdir, partname, curr->data, curr->size) == ERROR) {
				return ERROR;
			}
			printf("%s\n", partname);
			curr = curr->next;
		}
	}
	if (rawml->flow != NULL) {
		/* Linked list of MOBIPart structures in rawml->flow holds supplementary text files */
		MOBIPart* curr = rawml->flow;
		/* skip raw html file */
		curr = curr->next;
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
			snprintf(partname, sizeof(partname), "flow%05zu.%s", curr->uid, file_meta.extension);
			if (write_to_dir(newdir, partname, curr->data, curr->size) == ERROR) {
				return ERROR;
			}
			printf("%s\n", partname);
			curr = curr->next;
		}
	}
	if (rawml->resources != NULL) {
		/* Linked list of MOBIPart structures in rawml->resources holds binary files, also opf files */
		MOBIPart* curr = rawml->resources;
		/* jpg, gif, png, bmp, font, audio, video also opf, ncx */
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
			if (curr->size > 0) {
				int n;
				if (create_epub_opt && file_meta.type == T_OPF) {
					n = snprintf(partname, sizeof(partname), "%s%ccontent.opf", newdir, separator);
				}
				else {
					n = snprintf(partname, sizeof(partname), "%s%cresource%05zu.%s", newdir, separator, curr->uid, file_meta.extension);
				}
				if (n < 0) {
					printf("Creating file name failed\n");
					return ERROR;
				}
				if ((size_t)n >= sizeof(partname)) {
					printf("File name too long: %s\n", partname);
					return ERROR;
				}

				if (create_epub_opt && file_meta.type == T_OPF) {
					printf("content.opf\n");
				}
				else {
					printf("resource%05zu.%s\n", curr->uid, file_meta.extension);
				}

				if (write_file(curr->data, curr->size, partname) == ERROR) {
					return ERROR;
				}

			}
			curr = curr->next;
		}
	}
	return SUCCESS;
}

#ifdef USE_XMLWRITER
/**
 @brief Bundle recreated source files into EPUB container

 This function is a simple example.
 In real world implementation one should validate and correct all input
 markup to check if it conforms to OPF and HTML specifications and
 correct all the issues.

 @param[in] rawml MOBIRawml structure holding parsed records
 @param[in] fullpath File path will be parsed to build basenames of dumped records
 @return SUCCESS or ERROR
 */
static int create_epub(const MOBIRawml* rawml, unsigned char** out_buffer, long* out_buffer_len) {
	if (rawml == NULL) {
		printf("Rawml structure not initialized\n");
		return ERROR;
	}

	/* create zip (epub) archive */
	mz_zip_archive zip;
	memset(&zip, 0, sizeof(mz_zip_archive));
	mz_bool mz_ret = mz_zip_writer_init_heap(&zip, 0, 4000000);
	if (!mz_ret) {
		printf("Could not initialize zip archive\n");
		return ERROR;
	}
	/* start adding files to archive */
	mz_ret = mz_zip_writer_add_mem(&zip, "mimetype", EPUB_MIMETYPE, sizeof(EPUB_MIMETYPE) - 1, MZ_NO_COMPRESSION);
	if (!mz_ret) {
		printf("Could not add mimetype\n");
		mz_zip_writer_end(&zip);
		return ERROR;
	}
	mz_ret = mz_zip_writer_add_mem(&zip, "META-INF/container.xml", EPUB_CONTAINER, sizeof(EPUB_CONTAINER) - 1, (mz_uint)MZ_DEFAULT_COMPRESSION);
	if (!mz_ret) {
		printf("Could not add container.xml\n");
		mz_zip_writer_end(&zip);
		return ERROR;
	}
	char partname[FILENAME_MAX];
	if (rawml->markup != NULL) {
		/* Linked list of MOBIPart structures in rawml->markup holds main text files */
		MOBIPart* curr = rawml->markup;
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
			snprintf(partname, sizeof(partname), "OEBPS/part%05zu.%s", curr->uid, file_meta.extension);
			mz_ret = mz_zip_writer_add_mem(&zip, partname, curr->data, curr->size, (mz_uint)MZ_DEFAULT_COMPRESSION);
			if (!mz_ret) {
				printf("Could not add file to archive: %s\n", partname);
				mz_zip_writer_end(&zip);
				return ERROR;
			}
			curr = curr->next;
		}
	}
	if (rawml->flow != NULL) {
		/* Linked list of MOBIPart structures in rawml->flow holds supplementary text files */
		MOBIPart* curr = rawml->flow;
		/* skip raw html file */
		curr = curr->next;
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
			snprintf(partname, sizeof(partname), "OEBPS/flow%05zu.%s", curr->uid, file_meta.extension);
			mz_ret = mz_zip_writer_add_mem(&zip, partname, curr->data, curr->size, (mz_uint)MZ_DEFAULT_COMPRESSION);
			if (!mz_ret) {
				printf("Could not add file to archive: %s\n", partname);
				mz_zip_writer_end(&zip);
				return ERROR;
			}
			curr = curr->next;
		}
	}
	if (rawml->resources != NULL) {
		/* Linked list of MOBIPart structures in rawml->resources holds binary files, also opf files */
		MOBIPart* curr = rawml->resources;
		/* jpg, gif, png, bmp, font, audio, video, also opf, ncx */
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
			if (curr->size > 0) {
				if (file_meta.type == T_OPF) {
					snprintf(partname, sizeof(partname), "OEBPS/content.opf");
				}
				else {
					snprintf(partname, sizeof(partname), "OEBPS/resource%05zu.%s", curr->uid, file_meta.extension);
				}
				mz_ret = mz_zip_writer_add_mem(&zip, partname, curr->data, curr->size, (mz_uint)MZ_DEFAULT_COMPRESSION);
				if (!mz_ret) {
					printf("Could not add file to archive: %s\n", partname);
					mz_zip_writer_end(&zip);
					return ERROR;
				}
			}
			curr = curr->next;
		}
	}
	/* Finalize epub archive */
	void* buf;
	size_t size;

	mz_ret = mz_zip_writer_finalize_heap_archive(&zip, &buf, &size);
	if (!mz_ret) {
		printf("Could not finalize zip archive\n");
		mz_zip_writer_end(&zip);
		return ERROR;
	}

	*out_buffer = LocalAlloc(LPTR, size);
	*out_buffer_len = size;
	MoveMemory(*out_buffer, buf, size);

	if (buf) {
		zip.m_pFree(zip.m_pAlloc_opaque, buf);
	}

	mz_ret = mz_zip_writer_end(&zip);
	if (!mz_ret) {
		printf("Could not finalize zip writer\n");
		return ERROR;
	}
	return SUCCESS;
}
#endif

/**
 @brief Dump SRCS record
 @param[in] m MOBIData structure
 @param[in] fullpath Full file path
 @return SUCCESS or ERROR
 */
static int dump_embedded_source(const MOBIData* m, const char* fullpath) {
	/* Try to get embedded source */
	unsigned char* data = NULL;
	size_t size = 0;
	MOBI_RET mobi_ret = mobi_get_embedded_source(&data, &size, m);
	if (mobi_ret != MOBI_SUCCESS) {
		printf("Extracting source from mobi failed (%s)\n", libmobi_msg(mobi_ret));
		return ERROR;
	}
	if (data == NULL || size == 0) {
		printf("Source archive not found\n");
		return SUCCESS;
	}

	char newdir[FILENAME_MAX];
	if (create_dir(newdir, sizeof(newdir), fullpath, "_source") == ERROR) {
		return ERROR;
	}

	const unsigned char epub_magic[] = "mimetypeapplication/epub+zip";
	const size_t em_offset = 30;
	const size_t em_size = sizeof(epub_magic) - 1;
	const char* ext;
	if (size > em_offset + em_size && memcmp(data + em_offset, epub_magic, em_size) == 0) {
		ext = "epub";
	}
	else {
		ext = "zip";
	}

	char srcsname[FILENAME_MAX];
	char basename[FILENAME_MAX];
	split_fullpath(fullpath, NULL, basename, FILENAME_MAX);
	int n = snprintf(srcsname, sizeof(srcsname), "%s_source.%s", basename, ext);
	if (n < 0) {
		printf("Creating file name failed\n");
		return ERROR;
	}
	if ((size_t)n >= sizeof(srcsname)) {
		printf("File name too long\n");
		return ERROR;
	}
	if (write_to_dir(newdir, srcsname, data, size) == ERROR) {
		return ERROR;
	}
	printf("Saving source archive to %s\n", srcsname);

	/* Try to get embedded conversion log */
	data = NULL;
	size = 0;
	mobi_ret = mobi_get_embedded_log(&data, &size, m);
	if (mobi_ret != MOBI_SUCCESS) {
		printf("Extracting conversion log from mobi failed (%s)\n", libmobi_msg(mobi_ret));
		return ERROR;
	}
	if (data == NULL || size == 0) {
		printf("Conversion log not found\n");
		return SUCCESS;
	}

	n = snprintf(srcsname, sizeof(srcsname), "%s_source.txt", basename);
	if (n < 0) {
		printf("Creating file name failed\n");
		return ERROR;
	}
	if ((size_t)n >= sizeof(srcsname)) {
		printf("File name too long\n");
		return ERROR;
	}
	if (write_to_dir(newdir, srcsname, data, size) == ERROR) {
		return ERROR;
	}
	printf("Saving conversion log to %s\n", srcsname);

	return SUCCESS;
}

/**
 @brief Split hybrid file in two parts
 @param[in] fullpath Full file path
 @return SUCCESS or ERROR
 */
static int split_hybrid(const char* fullpath) {

	static int run_count = 0;
	run_count++;

	bool use_kf8 = run_count == 1 ? false : true;

	/* Initialize main MOBIData structure */
	MOBIData* m = mobi_init();
	if (m == NULL) {
		printf("Memory allocation failed\n");
		return ERROR;
	}

	errno = 0;
	FILE* file = fopen(fullpath, "rb");
	if (file == NULL) {
		int errsv = errno;
		printf("Error opening file: %s (%s)\n", fullpath, strerror(errsv));
		mobi_free(m);
		return ERROR;
	}
	/* MOBIData structure will be filled with loaded document data and metadata */
	MOBI_RET mobi_ret = mobi_load_file(m, file);
	fclose(file);

	if (mobi_ret != MOBI_SUCCESS) {
		printf("Error while loading document (%s)\n", libmobi_msg(mobi_ret));
		mobi_free(m);
		return ERROR;
	}

	mobi_ret = mobi_remove_hybrid_part(m, use_kf8);
	if (mobi_ret != MOBI_SUCCESS) {
		printf("Error removing hybrid part (%s)\n", libmobi_msg(mobi_ret));
		mobi_free(m);
		return ERROR;
	}

	if (save_mobi(m, fullpath, "split") != SUCCESS) {
		printf("Error saving file\n");
		mobi_free(m);
		return ERROR;
	}

	/* Free MOBIData structure */
	mobi_free(m);

	/* Proceed with KF8 part */
	if (use_kf8 == false) {
		split_hybrid(fullpath);
	}

	return SUCCESS;
}

/**
 @brief Main routine that calls optional subroutines
 @param[in] fullpath Full file path
 @return SUCCESS or ERROR
 */
static int loadmemory(const unsigned char* buffer, long buffer_len, unsigned char** out_buffer, long* out_buffer_len) {
	MOBI_RET mobi_ret;
	int ret = SUCCESS;
	/* Initialize main MOBIData structure */
	MOBIData* m = mobi_init();
	if (m == NULL) {
		printf("Memory allocation failed\n");
		return ERROR;
	}
	/* By default loader will parse KF8 part of hybrid KF7/KF8 file */
	if (parse_kf7_opt) {
		/* Force it to parse KF7 part */
		mobi_parse_kf7(m);
	}
	errno = 0;

	MEMORY_FILE mf;
	mf.current_file_position = 0;
	mf.file_length = buffer_len;
	mf.file_buffer = buffer;

	/* MOBIData structure will be filled with loaded document data and metadata */
	mobi_ret = mobi_load_file_memory(m, &mf);


	/* Try to print basic metadata, even if further loading failed */
	/* In case of some unsupported formats it may still print some useful info */
	if (print_extended_meta_opt) { print_meta(m); }

	if (mobi_ret != MOBI_SUCCESS) {
		printf("Error while loading document (%s)\n", libmobi_msg(mobi_ret));
		mobi_free(m);
		return ERROR;
	}

	if (create_epub_opt && mobi_is_replica(m)) {
		create_epub_opt = false;
		printf("\nWarning: Can't create EPUB format from Print Replica book (ignoring -e argument)\n\n");
	}

	if (!print_extended_meta_opt) {
		print_summary(m);
	}

	if (print_extended_meta_opt) {
		/* Try to print EXTH metadata */
		print_exth(m);
	}

#ifdef USE_ENCRYPTION
	if (setpid_opt || setserial_opt) {
		ret = set_decryption_key(m, serial, pid);
		if (ret != SUCCESS) {
			mobi_free(m);
			return ret;
		}
	}
#endif
	if (print_rec_meta_opt) {
		printf("\nPrinting records metadata...\n");
		print_records_meta(m);
	}
	if (dump_parts_opt || create_epub_opt) {
		printf("\nReconstructing source resources...\n");
		/* Initialize MOBIRawml structure */
		/* This structure will be filled with parsed records data */
		MOBIRawml* rawml = mobi_init_rawml(m);
		if (rawml == NULL) {
			printf("Memory allocation failed\n");
			mobi_free(m);
			return ERROR;
		}

		/* Parse rawml text and other data held in MOBIData structure into MOBIRawml structure */
		mobi_ret = mobi_parse_rawml(rawml, m);
		if (mobi_ret != MOBI_SUCCESS) {
			printf("Parsing rawml failed (%s)\n", libmobi_msg(mobi_ret));
			mobi_free(m);
			mobi_free_rawml(rawml);
			return ERROR;
		}
		if (create_epub_opt && !dump_parts_opt) {
#ifdef USE_XMLWRITER
			printf("\nCreating EPUB...\n");
			/* Create epub file */
			ret = create_epub(rawml, out_buffer, out_buffer_len);
			if (ret != SUCCESS) {
				printf("Creating EPUB failed\n");
			}
#endif
		}
		/* Free MOBIRawml structure */
		mobi_free_rawml(rawml);
	}
	if (split_opt && !mobi_is_hybrid(m)) {
		printf("File is not a hybrid, skip splitting\n");
		split_opt = false;
	}
	/* Free MOBIData structure */
	mobi_free(m);
	return ret;
}


/**
 @brief Main

 @param[in] argc Arguments count
 @param[in] argv Arguments array
 @return SUCCESS (0) or ERROR (1)
 */
__declspec(dllexport) int __cdecl ConvertMobiToEpub(unsigned char* buffer, long buf_len, unsigned char** out_buffer, long* out_buffer_len) {
	create_epub_opt = true;
	outdir_opt = true;

	int ret = SUCCESS;

	ret = loadmemory(buffer, buf_len, out_buffer, out_buffer_len);

	return ret;
}