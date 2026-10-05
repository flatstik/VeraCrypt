// Decrypt the data area of a volume through the volume layer, without
// mounting. Usage: volume_read_test VOLUME PASSWORD KDF OUTPUT
//
// The volume is opened read-only with PIM 0 and the given KDF (a hash name,
// as with --hash). The data area is read in one call and again one sector at
// a time; both must match. The plaintext is written to OUTPUT, where
// Tests/test_volume_read.py checks it.
#include "Platform/Platform.h"
#include "Volume/Volume.h"
#include "Volume/Pkcs5Kdf.h"
#include <cstring>
#include <fstream>
#include <iostream>

using namespace VeraCrypt;

// Same matching as --hash on the command line: the hash name or its
// alternative name, or "argon2"/"argon2id".
static shared_ptr <Pkcs5Kdf> FindKdf (const string &name)
{
	string lowerName = StringConverter::ToLower (name);
	foreach (shared_ptr <Pkcs5Kdf> kdf, Pkcs5Kdf::GetAvailableAlgorithms())
	{
		if (kdf->IsArgon2())
		{
			if (lowerName == "argon2" || lowerName == "argon2id")
				return kdf;
			continue;
		}
		if (StringConverter::ToLower (StringConverter::ToSingle (kdf->GetHash()->GetName())) == lowerName
			|| StringConverter::ToLower (StringConverter::ToSingle (kdf->GetHash()->GetAltName())) == lowerName)
			return kdf;
	}
	throw ParameterIncorrect (SRC_POS);
}

int main (int argc, char **argv)
{
	if (argc != 5)
	{
		std::cerr << "usage: volume_read_test VOLUME PASSWORD KDF OUTPUT\n";
		return 2;
	}

	try
	{
		shared_ptr <VolumePassword> password (new VolumePassword ((const uint8 *) argv[2], strlen (argv[2])));
		Volume volume;
		volume.Open (VolumePath (string (argv[1])), true, password, 0, FindKdf (argv[3]),
			shared_ptr <KeyfileList> (), false, VolumeProtection::ReadOnly);

		const uint64 size = volume.GetSize();
		const size_t sectorSize = volume.GetSectorSize();
		if (size == 0 || size % sectorSize != 0)
			throw ParameterIncorrect (SRC_POS);

		SecureBuffer data ((size_t) size);
		volume.ReadSectors (data, 0);

		SecureBuffer sector (sectorSize);
		for (uint64 offset = 0; offset < size; offset += sectorSize)
		{
			volume.ReadSectors (sector, offset);
			if (memcmp (sector.Ptr(), data.Ptr() + offset, sectorSize) != 0)
			{
				std::cerr << "sector at " << offset << " differs from the same bytes read in one call\n";
				return 1;
			}
		}

		std::ofstream output (argv[4], std::ios::binary);
		output.write ((const char *) data.Ptr(), (std::streamsize) data.Size());
		output.close();
		if (!output)
			throw ParameterIncorrect (SRC_POS);

		std::cout << StringConverter::ToSingle (volume.GetEncryptionAlgorithm()->GetName())
			<< " " << size << " bytes\n";
		volume.Close();
	}
	catch (std::exception &e)
	{
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
