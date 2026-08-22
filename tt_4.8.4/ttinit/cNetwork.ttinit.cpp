#include "General.h"
#include "cNetwork.h"



#include "engine_io.h"
#pragma warning(disable:6262) // Function uses '' bytes of stack: exceeds /analyze:stacksize



uint32 cNetwork::Get_Data_Files_CRC()
{
	static int dataFilesCrc = computeDataFilesCrc();
	return dataFilesCrc;
}



uint32 cNetwork::computeDataFilesCrc()
{
	static const char* filenames[] =
	{		
		"jgo`fqv+aag",         // "objects.ddb",
		"dwhjw+lkl",           // "armor.ini",
		"gjk`v+lkl",           // "bones.ini",
		"vpwcdf``cc`fqv+lkl",  // "surfaceeffects.ini",
		"fdh`wdv+lkl",         // "cameras.ini",
		"fZkjaZhbZi5+r6a",     // "c_nod_mg_l0.w3d",
		"fZkjaZwnZi5+r6a",     // "c_nod_rk_l0.w3d",
		"fZkjaZciZi5+r6a",     // "c_nod_fl_l0.w3d",
		"fZkjaZ`kZi5+r6a",     // "c_nod_en_l0.w3d",
		"fZkjaZhbjZi5+r6a",    // "c_nod_mgo_l0.w3d",
		"fZkjaZwnjZi5+r6a",    // "c_nod_rko_l0.w3d",
		"fZkjaZfm`hqZi5+r6a",  // "c_nod_chemt_l0.w3d",
		"fZkjaZvklu`wZi5+r6a", // "c_nod_sniper_l0.w3d",
		"fZkjaZwvjiaZi5+r6a",  // "c_nod_rsold_l0.w3d",
		"fZkjaZvqiqmZi5+r6a",  // "c_nod_stlth_l0.w3d",
		"fZkjaZvdnpZi5+r6a",   // "c_nod_saku_l0.w3d",
		"fZkjaZvdnp7Zi5+r6a",  // "c_nod_saku2_l0.w3d",
		"fZkjaZwdsZi5+r6a",    // "c_nod_rav_l0.w3d",
		"fZkjaZhwdsZi5+r6a",   // "c_nod_mrav_l0.w3d",
		"fZkjaZhaZi5+r6a",    // "c_nod_mdz_l0.w3d",
		"fZkjaZha7Zi5+r6a",   // "c_nod_mdz2_l0.w3d",
		"fZkjaZqfZi5+r6a",     // "c_nod_tc_l0.w3d",
		"fZkjaZhpqdkqZi5+r6a", // "c_nod_mutant_l0.w3d",
		"fZkjaZhviaZi5+r6a",   // "c_nod_msld_l0.w3d",
		"fZkjaZvvjiaZi5+r6a",  // "c_nod_ssold_l0.w3d",
		"fZkjaZu`qhZi5+r6a",   // "c_nod_petm_l0.w3d",
		"fZkjaZndk`Zi5+r6a",   // "c_nod_kane_l0.w3d",
		"fZbalZhbZi5+r6a",     // "c_gdi_mg_l0.w3d",
		"fZbalZwnZi5+r6a",     // "c_gdi_rk_l0.w3d",
		"fZbalZbwZi5+r6a",     // "c_gdi_gr_l0.w3d",
		"fZbalZ`kZi5+r6a",     // "c_gdi_en_l0.w3d",
		"fZbalZhbjZi5+r6a",    // "c_gdi_mgo_l0.w3d",
		"fZbalZwnjZi5+r6a",    // "c_gdi_rko_l0.w3d",
		"fZbalZv|aZi5+r6a",    // "c_gdi_syd_l0.w3d",
		"fZbalZa`daZi5+r6a",   // "c_gdi_dead_l0.w3d",
		"fZbalZbpkZi5+r6a",    // "c_gdi_gun_l0.w3d",
		"fZbalZuqfmZi5+r6a",   // "c_gdi_ptch_l0.w3d",
		"fZmdsjfZi5+r6a",      // "c_havoc_l0.w3d",
		"fZmdsjfkZi5+r6a",     // "c_havocn_l0.w3d",
		"fZmdsjfrZi5+r6a",     // "c_havocw_l0.w3d",
		"fZmdsjfaZi5+r6a",     // "c_havocd_l0.w3d",
		"fZbalZv|aZi5+r6a",    // "c_gdi_syd_l0.w3d",
		"fZbalZv|a7Zi5+r6a",   // "c_gdi_syd2_l0.w3d",
		"fZbalZhjglZi5+r6a",   // "c_gdi_mobi_l0.w3d",
		"fZbalZmjqrZi5+r6a",   // "c_gdi_hotw_l0.w3d",
		"fZbalZiqZi5+r6a",     // "c_gdi_lt_l0.w3d",
		"fZkjaZu`qwZi5+r6a",   // "c_nod_petr_l0.w3d",
		"fZijbdkZi5+r6a",      // "c_logan_l0.w3d",
		"fZbalZijfn`Zi5+r6a",  // "c_gdi_locke_l0.w3d",
		"sZkjaZgpbb|+r6a",     // "v_nod_buggy.w3d",
		"sZkjaZdufZh+r6a",     // "v_nod_apc_m.w3d",
		"sZkjaZdwqiw|+r6a",    // "v_nod_artlry.w3d",
		"sZkjaZcidh`+r6a",     // "v_nod_flame.w3d",
		"sZkjaZiqdkn+r6a",     // "v_nod_ltank.w3d",
		"sZkjaZvqiqm+r6a",     // "v_nod_stlth.w3d",
		"sZkjaZqwkvuqZh+r6a",  // "v_nod_trnspt_m.w3d",
		"sZkjaZdudfm`Zh+r6a",  // "v_nod_apache_m.w3d",
		"sZfmdh`i`jk+r6a",     // "v_chameleon.w3d",
		"sZbalZmphs``+r6a",    // "v_gdi_humvee.w3d",
		"sZbalZdufZh+r6a",     // "v_gdi_apc_m.w3d",
		"sZbalZhwiv+r6a",      // "v_gdi_mrls.w3d",
		"sZbalZh`aqkn+r6a",    // "v_gdi_medtnk.w3d",
		"sZbalZhdhhqm+r6a",    // "v_gdi_mammth.w3d",
		"sZulfnpu54+r6a",      // "v_pickup01.w3d",
		"sZv`adk54+r6a",       // "v_sedan01.w3d",
		"sZbalZjwfdZh+r6a",    // "v_gdi_orca_m.w3d",
		"sZbalZqwkvuqZh+r6a",  // "v_gdi_trnspt_m.w3d",
	};
	static const int fileCount = _countof(filenames);
	
	int crc = 0;
	for (int i = 0; i < fileCount; i++)
	{
		StringClass filename = filenames[i];

		char* filenameIter = filename.Peek_Buffer();
		while (*filenameIter != '\0')
			*filenameIter++ ^= 5;

		FileClass* file = Get_Data_File(filename);
		if (file)
		{
			if (file->Is_Available())
			{
				file->Open();

				uint8 buffer[0x4000];
				uint bufferLength;
				do
				{
					bufferLength = file->Read(buffer, sizeof(buffer));
					crc = CRC_Memory(buffer, bufferLength, crc);
				} while (bufferLength > 0);

				file->Close();
			}

			Close_Data_File(file);
		}
	}

	return crc;
}