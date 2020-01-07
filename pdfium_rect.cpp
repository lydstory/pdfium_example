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
struct PdfToFdWriter : FPDF_FILEWRITE {
    int dstFd;
};

static bool writeAllBytes(const int fd, const void *buffer, const size_t byteCount) {
    char *writeBuffer = static_cast<char *>(const_cast<void *>(buffer));
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
static int writeBlock(FPDF_FILEWRITE* owner, const void* buffer, unsigned long size) {
    const PdfToFdWriter* writer = reinterpret_cast<PdfToFdWriter*>(owner);
    const bool success = writeAllBytes(writer->dstFd, buffer, size);
    if (success < 0) {
        return 0;
    }
    return 1;
}

//FPDF_DOCUMENT document_;
int main(int argc, const char* argv[])
{

	 FPDF_InitLibrary();
	 FPDF_DOCUMENT outdoc;
	  string inpdf = "/home/lyd/work/gnpdf/441.pdf";
	  		 outdoc = FPDF_CreateNewDocument();
	  		 FPDF_PAGE page = FPDFPage_New(outdoc, 0, 612, 792);

	  		  FPDF_PAGEOBJECT red_rect = FPDFPageObj_CreateNewRect(10, 10, 20, 20);

	  		  // Expect false when trying to set colors out of range
	  		FPDFPath_SetStrokeColor(red_rect, 100, 100, 100, 300);
	  		FPDFPath_SetFillColor(red_rect, 200, 256, 200, 0);

	  		  // Fill rectangle with red and insert to the page
	  		FPDFPath_SetFillColor(red_rect, 255, 0, 0, 255);
	  		 FPDFPath_SetDrawMode(red_rect, FPDF_FILLMODE_ALTERNATE, 0);
	  		  FPDFPage_InsertObject(page, red_rect);
	  		  FPDFPage_GenerateContent(page);
	     	 string filename2 = "/home/lyd/work/gnpdf/me.pdf";
	  		 int fp = open(filename2.c_str(), O_RDWR|O_CREAT);
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
