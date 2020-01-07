#include <iostream>
using namespace std;
#include <map>
#include <memory>
#include <string>
#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/fx_system.h"
#include "fpdfsdk/fsdk_define.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_ext.h"
#include "public/fpdf_formfill.h"
#include "public/fpdf_save.h"
#include "public/fpdfview.h"
#include "public/fpdf_edit.h"
#include "public/fpdf_text.h"

#include "public/fpdf_ppo.h"
extern "C" {
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
}
#include <fcntl.h>

#include <stdlib.h>

#include <vector>

struct PdfToFdWriter: FPDF_FILEWRITE {
	int dstFd;
};

static bool writeAllBytes(const int fd, const void *buffer,
		const size_t byteCount) {
	char *writeBuffer = static_cast<char*>(const_cast<void*>(buffer));
	size_t remainingBytes = byteCount;
	while (remainingBytes > 0) {
		ssize_t writtenByteCount = write(fd, writeBuffer, remainingBytes);
		if (writtenByteCount == -1) {
			return false;
		}
		remainingBytes -= writtenByteCount;
		writeBuffer += writtenByteCount;
	}
	return true;
}
static int writeBlock(FPDF_FILEWRITE *owner, const void *buffer,
		unsigned long size) {
	const PdfToFdWriter *writer = reinterpret_cast<PdfToFdWriter*>(owner);
	const bool success = writeAllBytes(writer->dstFd, buffer, size);
	if (success < 0) {
		return 0;
	}
	return 1;
}
namespace pdfium {

// Used with std::unique_ptr to free() objects that can't be deleted.
struct FreeDeleter {
	inline void operator()(void *ptr) const {
		free(ptr);
	}
};
}
std::unique_ptr<char, pdfium::FreeDeleter> GetFileContents(const char *filename,
		size_t *retlen) {
	FILE *file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Failed to open: %s\n", filename);
		return nullptr;
	}
	(void) fseek(file, 0, SEEK_END);
	size_t file_length = ftell(file);
	if (!file_length) {
		return nullptr;
	}
	(void) fseek(file, 0, SEEK_SET);
	std::unique_ptr<char, pdfium::FreeDeleter> buffer(
			static_cast<char*>(malloc(file_length)));
	if (!buffer) {
		return nullptr;
	}
	size_t bytes_read = fread(buffer.get(), 1, file_length, file);
	(void) fclose(file);
	if (bytes_read != file_length) {
		fprintf(stderr, "Failed to read: %s\n", filename);
		return nullptr;
	}
	*retlen = bytes_read;
	return buffer;
}
std::unique_ptr<unsigned short, pdfium::FreeDeleter> GetFPDFWideString(
		const std::wstring &wstr) {
	size_t length = sizeof(uint16_t) * (wstr.length() + 1);
	std::unique_ptr<unsigned short, pdfium::FreeDeleter> result(
			static_cast<unsigned short*>(malloc(length)));
	char *ptr = reinterpret_cast<char*>(result.get());
	size_t i = 0;
	for (wchar_t w : wstr) {
		ptr[i++] = w & 0xff;
		ptr[i++] = (w >> 8) & 0xff;
	}
	ptr[i++] = 0;
	ptr[i] = 0;
	return result;
}
//FPDF_DOCUMENT document_;
int main(int argc, const char *argv[]) {

	FPDF_InitLibrary();
	FPDF_DOCUMENT outdoc;
	string inpdf = "/home/lyd/work/gnpdf/441.pdf";

	outdoc = FPDF_CreateNewDocument();
	FPDF_PAGE page = FPDFPage_New(outdoc, 0, 612, 792);

	FPDF_PAGEOBJECT text_object1 = FPDFPageObj_NewTextObj(outdoc, "Arial",
			12.0f);
	std::unique_ptr<unsigned short, pdfium::FreeDeleter> text1 =
			GetFPDFWideString(L"I'm at the bottom of the page");
	FPDFText_SetText(text_object1, text1.get());

	FPDFPageObj_Transform(text_object1, 1, 0, 0, 1, 20, 20);
	FPDFPage_InsertObject(page, text_object1);

	FPDFPage_GenerateContent(page);
	string filename2 = "/home/lyd/work/gnpdf/me.pdf";
	int fp = open(filename2.c_str(), O_RDWR | O_CREAT);
	PdfToFdWriter writer;
	writer.version = 1;
	writer.dstFd = fp;
	writer.WriteBlock = &writeBlock;
	FPDF_SaveAsCopy(outdoc, &writer, FPDF_NO_INCREMENTAL);

	FPDF_CloseDocument(outdoc);
	close(fp);

	FPDF_DestroyLibrary();

	return 0;
}
