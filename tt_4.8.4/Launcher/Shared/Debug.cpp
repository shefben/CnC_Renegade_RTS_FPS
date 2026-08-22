#ifdef _DEBUG
#include "Memory.h"
#include "MemoryStream.h"
#include "HTMLWrite.h"

IStream* GenerateMemStatsPage(){
	MemStats stats;
	
	CHTMLWriter *pWriter;
	CMemoryBlockStream* pStream;
	memcpy(&stats, &g_MemStats, sizeof(MemStats));

	if (!(pStream = new CMemoryBlockStream(8192))) return NULL;
	if (!(pWriter = new CHTMLWriter(pStream))){
		pStream->Release();
		return NULL;
	}
	pWriter->OpenTag(L"html");

	pWriter->AddAttribute(L"bgcolor", L"#000000");
	//link="#C0C0C0" vlink="#808080" alink="#FF0000"
	pWriter->AddAttribute(L"link", L"#B0B0B0");
	pWriter->AddAttribute(L"vlink", L"#A0A0A0");
	pWriter->AddAttribute(L"alink", L"#000000");
	pWriter->OpenTag(L"body");


	pWriter->AddAttribute(L"size", L"8");
	pWriter->AddAttribute(L"color", L"#FFFFFF");
	pWriter->OpenTag(L"font");

	pWriter->OpenTag(L"b");
	pWriter->WriteString(L"Memory usage statistics:");
	pWriter->CloseTag(); //<b>
	pWriter->WriteClosedTag(L"br");
	pWriter->WriteClosedTag(L"br");
	pWriter->WriteClosedTag(L"br");

	pWriter->CloseTag(); //<font>


	
	pWriter->AddAttribute(L"size", L"6");
	pWriter->AddAttribute(L"color", L"#000000");
	pWriter->OpenTag(L"font");

	pWriter->AddAttribute(L"bgcolor", L"#D8D8AF");

	pWriter->OpenTag(L"table");

	//Current usage
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Current memory usage: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%.3f KB", (float)(((double)stats.m_CurrentUsage)/1024));
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Current alloc count
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Current alloc count: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_CurrentAllocs);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Allocated
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Allocated memory: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%.3f KB", (float)(((double)stats.m_AllocatedMemory)/1024));
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Reallocated
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Reallocated memory: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%.3f KB", (float)(((double)stats.m_ReallocatedMemory)/1024));
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Freed
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Freed memory: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%.3f KB", (float)(((double)stats.m_FreedMemory)/1024));
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Num allocs
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Number of allocs: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_NumAllocs);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Num allocs
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Number of Reallocs: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_NumReallocs);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Num frees
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Number of frees: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_NumFrees);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>


	//Num new allocations
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Number of current new allactions: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_NumNewAllocations);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Num new
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Number of news: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_NumNew);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Num delete
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Number of deletes: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_NumDelete);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Num array new allocations
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Number of current array new allactions: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_NumANewAllocations);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Num array new
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Number of array news: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_NumANew);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>

	//Num array delete
	pWriter->OpenTag(L"tr");
	pWriter->OpenTag(L"td");
	pWriter->WriteString(L"Number of array deletes: ");
	pWriter->CloseTag(); //<td>
	pWriter->AddAttribute(L"align", L"right");
	pWriter->OpenTag(L"td");
	pWriter->WriteStringFmt(L"%I64u", (size_64)stats.m_NumADelete);
	pWriter->CloseTag(); //<td>
	pWriter->CloseTag(); //<tr>


	pWriter->CloseTag(); //<table>
	pWriter->CloseTag(); //<font>

	pWriter->AddAttribute(L"href", L"dyn://memstats");
	pWriter->OpenTag(L"a");
	pWriter->OpenTag(L"b");
	pWriter->WriteString(L"Refresh");
	pWriter->WriteClosedTag(L"br");
	pWriter->CloseTag();
	pWriter->CloseTag();




	pWriter->CloseTag(); //<body>
	pWriter->CloseTag(); //<html>
	pWriter->Finalize();
	delete pWriter;

	return pStream;
}
#endif //_DEBUG
