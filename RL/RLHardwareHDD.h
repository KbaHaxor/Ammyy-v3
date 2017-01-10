#if !defined(RL_HARDWARE_HDD_6341A1__INCLUDED_)
#define RL_HARDWARE_HDD_6341A1__INCLUDED_

#include "RLFile.h"
#include "RLHardwareHDD_ddk.h"
#include <Shlwapi.h>

#pragma comment( lib, "Shlwapi" )

class RLHardwareHDD
{
public:	
	class HDD
	{
	public:
		void Clear()
		{
			firmwareRev = "";
			model  = "";
			serial = "";
			size = 0;
		}

		bool IsValid()
		{
			if (size<=0) return false;

			CStringA serial1 = serial; // need to save original
			serial1.Replace('\0', ' '); // for Trim work correctly
			serial1.TrimRight();
			serial1.TrimLeft();
			return (!serial1.IsEmpty());
		}

		CStringA firmwareRev; // productRevision or BIOS revision
		CStringA model;		  // vendorId + productId
		CStringA serial;
		INT64	 size;
	};

	//  function to decode the serial numbers of IDE hard drives using the IOCTL_STORAGE_QUERY_PROPERTY command 
	static CStringA FlipAndCodeBytes (const char * str, int pos, bool flip, bool trim)
	{
		CStringA out;
		bool done = false;
		int k = 0;

		if (pos <= 0)
			return out;

		str += pos;

		char* buf = out.GetBuffer(strlen(str)+1);
		
		{
			int p = 0;

			// First try to gather all characters representing hex digits only.
			done = true;
			k = 0;
			buf[k] = 0;
			for (LPCSTR pSrc = str; true;)
			{
				char c = *pSrc++;
				if (c==0) break;
				c = tolower(c);

				if (isspace(c))
					c = '0';

				p++;
				buf[k] <<= 4;

				if (c >= '0' && c <= '9')
					buf[k] |= (unsigned char) (c - '0');
				else if (c >= 'a' && c <= 'f')
					buf[k] |= (unsigned char) (c - 'a' + 10);
				else
				{
					done = false;
					break;
				}

				if (p == 2)
				{
					if (buf[k] != '\0' && ! isprint(buf[k]))
					{
						done = false;
						break;
					}
					k++;
					p = 0;
					buf[k] = 0;
				}
			}
		}

		if (!done)
		{
			// There are non-digit characters, gather them as is.
			done = true;
			k = 0;
			for (LPCSTR pSrc = str; true;)
			{
				char c = *pSrc++;
				if (c==0) break;

				if ( ! isprint(c))
				{
					done = false;
					break;
				}
				buf[k++] = c;
			}
		}

		if (!done)
		{
			// The characters are not there or are not printable.
			k = 0;
		}

		if (flip) {
			// Flip adjacent characters
			for (int j = 0; j < k; j += 2)
			{
				char t = buf[j];
				buf[j] = buf[j + 1];
				buf[j + 1] = t;
			}
		}

		out.ReleaseBuffer(k);
		out.MakeUpper();

		if (trim) {
			out.TrimRight();
			out.TrimLeft();
		}

		return out;
	}

	static DWORD MyDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize,
									RLStream& bufferOut, DWORD& dwBytesReturned)
	{
	begin:
		dwBytesReturned = 0;
		memset(bufferOut.GetBuffer(), 0, bufferOut.GetCapacity());
		if (::DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, 
									bufferOut.GetBuffer(), bufferOut.GetCapacity(), &dwBytesReturned, NULL))
		{
			return 0; // means OK
		}

		DWORD dwError = ::GetLastError();
		if((ERROR_INSUFFICIENT_BUFFER == dwError) || (ERROR_MORE_DATA == dwError))
		{
			bufferOut.SetMinCapasity(2 * bufferOut.GetCapacity()); // increase in 2 times
			goto begin;
		}
		return dwError;		
	}



	//  Windows NT, Windows 2000, Windows XP - admin rights not required
	//  maximp: In WinXP sometime return empty 'serial'
	static bool ReadPhysicalDriveWithZeroRights(DWORD dwDrive, HDD& hdd)
	{
		hdd.Clear();

		DWORD dwBytesReturned;		

		char szDriveName[64];
		sprintf(szDriveName, "\\\\.\\PhysicalDrive%u", dwDrive);

		HANDLE hDrive = ::CreateFile(szDriveName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hDrive == INVALID_HANDLE_VALUE)
			return false;
			//throw RLException("GetDiskInfo() %u %u", 1, ::GetLastError());

		RLHandle hDriverWrapper(hDrive); // for closing handle in descructor in case of Exception

		//NEVER shall be less than offset of Size member of STORAGE_DEVICE_DESCRIPTOR + sizeof(ULONG)
		RLStream buffer(32*1024);
    
		STORAGE_PROPERTY_QUERY query;
		memset ((void *)&query, 0, sizeof (query));
		query.PropertyId = StorageDeviceProperty;
		query.QueryType = PropertyStandardQuery;

		
		while(true)
		{
			DWORD error = MyDeviceIoControl(hDrive, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), 
												buffer, dwBytesReturned);

			if (error!=0) return false; // error was occur

			DWORD dwDescrSize = ((STORAGE_DEVICE_DESCRIPTOR*)buffer.GetBuffer())->Size;

			if (dwDescrSize <= dwBytesReturned) break; // ok

			//Some drivers doesn't return error code even if buffer is too short
			//::SetLastError(ERROR_INSUFFICIENT_BUFFER);
			buffer.SetMinCapasity(2 * buffer.GetCapacity()); // increase in 2 times
		}

		char *pBuffer = (char*)buffer.GetBuffer();
		STORAGE_DEVICE_DESCRIPTOR* descrip = (STORAGE_DEVICE_DESCRIPTOR*) pBuffer;

		CStringA vendorId = FlipAndCodeBytes(pBuffer, descrip->VendorIdOffset, false, false );		
		vendorId.TrimLeft();

		hdd.model  = vendorId + FlipAndCodeBytes(pBuffer, descrip->ProductIdOffset, false, true );
		hdd.firmwareRev = FlipAndCodeBytes(pBuffer, descrip->ProductRevisionOffset, false, true );
		hdd.serial = FlipAndCodeBytes(pBuffer, descrip->SerialNumberOffset, true, true);

		DWORD error = MyDeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, buffer, dwBytesReturned);

		if (error!=0) return false; // error was occur

		hdd.size = ((DISK_GEOMETRY_EX*)buffer.GetBuffer())->DiskSize.QuadPart;

		return true;
	}

	// dwDrive - can be different for same HDD than other method
	//
	static void ReadIdeDriveAsScsiDrive(DWORD dwDrive, HDD& hdd)
	{
		hdd.Clear();

		INT controller = dwDrive / 2; // can be [0..15]
		INT drive      = dwDrive % 2; // can be [0..2]

		 //  Try to get a handle to PhysicalDrive IOCTL, report failure  and exit if can't.
		char szDriveName[64];
		sprintf (szDriveName, "\\\\.\\Scsi%d:", controller);

		//  Windows NT, Windows 2000, any rights should do
		HANDLE  hDrive = ::CreateFile (szDriveName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		if (hDrive == INVALID_HANDLE_VALUE) return;
			//throw RLException("ReadHD#2() %u %u", 1, ::GetLastError());

		RLHandle hDriverWrapper(hDrive); // for closing handle in descructor
		
		char buffer [sizeof (SRB_IO_CONTROL) + SENDIDLENGTH];
		SRB_IO_CONTROL *p = (SRB_IO_CONTROL *) buffer;
		SENDCMDINPARAMS *pin = (SENDCMDINPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
		DWORD dummy;
   
		memset (buffer, 0, sizeof (buffer));
		p -> HeaderLength = sizeof (SRB_IO_CONTROL);
		p -> Timeout = 10000;
		p -> Length = SENDIDLENGTH;
		p -> ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
		strncpy ((char *) p -> Signature, "SCSIDISK", 8);
  
		pin -> irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
		pin -> bDriveNumber = drive;

		if (::DeviceIoControl(hDrive, IOCTL_SCSI_MINIPORT, 
									 buffer, sizeof (SRB_IO_CONTROL) + sizeof (SENDCMDINPARAMS) - 1,
									 buffer, sizeof (SRB_IO_CONTROL) + SENDIDLENGTH,
									 &dummy, NULL))
		{
			SENDCMDOUTPARAMS *pOut = (SENDCMDOUTPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
			IDSECTOR *pId = (IDSECTOR *) (pOut -> bBuffer);
			if (pId -> sModelNumber [0])
			{          
				FillHDDInfo((UINT16*)pId, hdd);
			}					  
		}
	}

	static bool ReadPhysicalDriveWithAdminRights(DWORD drive, HDD& hdd)
	{
		hdd.Clear();

		char driveName[64];
		sprintf (driveName, "\\\\.\\PhysicalDrive%d", drive);

		//  Windows NT, Windows 2000, must have admin rights
		HANDLE hDrive = ::CreateFile (driveName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE , 
											NULL, OPEN_EXISTING, 0, NULL);

		if (hDrive == INVALID_HANDLE_VALUE) return false; // error
		RLHandle hDriverWrapper(hDrive); // for closing handle in descructor


		GETVERSIONOUTPARAMS VersionParams;
		DWORD               cbBytesReturned = 0;

		// Get the version, etc of PhysicalDrive IOCTL
		memset ((void*) &VersionParams, 0, sizeof(VersionParams));

		if ( !::DeviceIoControl(hDrive, DFP_GET_VERSION, NULL, 0, &VersionParams, sizeof(VersionParams), &cbBytesReturned, NULL))
		{
			return false; // error ::GetLastError()
		}

		// If there is a IDE device at number "i" issue commands  to the device
		if (VersionParams.bIDEDeviceMap <= 0)
		{
			return false;
		}		

		// Now, get the ID sector for all IDE devices in the system.
		// If the device is ATAPI use the IDE_ATAPI_IDENTIFY command, otherwise use the IDE_ATA_IDENTIFY command
		BYTE bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY; // IDE or ATAPI IDENTIFY cmd

		// Set up data structures for IDENTIFY command.
		SENDCMDINPARAMS  scip = {0};
		scip.cBufferSize = IDENTIFY_BUFFER_SIZE;
		scip.irDriveRegs.bFeaturesReg = 0;
		scip.irDriveRegs.bSectorCountReg = 1;
		//scip.irDriveRegs.bSectorNumberReg = 1;
		scip.irDriveRegs.bCylLowReg = 0;
		scip.irDriveRegs.bCylHighReg = 0;
		scip.irDriveRegs.bDriveHeadReg = (BYTE)(0xA0 | ((drive & 1) << 4));  // Compute the drive number.

		// The command can either be IDE identify or ATAPI identify.
		scip.irDriveRegs.bCommandReg = bIDCmd;
		scip.bDriveNumber = (BYTE)drive;
		scip.cBufferSize = IDENTIFY_BUFFER_SIZE;

		RLStream buffer(sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1);
		memset(buffer.GetBuffer(), 0, buffer.GetCapacity());

		// Send an IDENTIFY command to the drive
		if ( ::DeviceIoControl (hDrive, DFP_RECEIVE_DRIVE_DATA,
				   (LPVOID)&scip, sizeof(SENDCMDINPARAMS) - 1,
				   buffer.GetBuffer(), sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
				   &cbBytesReturned, NULL))
		{
			USHORT *pIdSector = (USHORT *)((PSENDCMDOUTPARAMS)buffer.GetBuffer()) -> bBuffer;
			FillHDDInfo(pIdSector, hdd);
			return true;
		}
		else
			return false; // error
	}

	// require admin rights
	static bool ReadPhysicalDriveInUsingSmart(DWORD drive, HDD& hdd)
	{
		hdd.Clear();

		char driveName [64];
		sprintf (driveName, "\\\\.\\PhysicalDrive%d", drive);

		 //  Windows NT, Windows 2000, Windows Server 2003, Vista
		HANDLE hDrive = ::CreateFile(driveName, GENERIC_READ | GENERIC_WRITE, 
								   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		if (hDrive == INVALID_HANDLE_VALUE) return false; // error
		RLHandle hDriverWrapper(hDrive); // for closing handle in descructor

		
		GETVERSIONINPARAMS GetVersionParams;
		DWORD cbBytesReturned = 0;

		// Get the version, etc of PhysicalDrive IOCTL
		memset ((void*) & GetVersionParams, 0, sizeof(GetVersionParams));

		if ( !::DeviceIoControl(hDrive, SMART_GET_VERSION, NULL, 0,  &GetVersionParams, sizeof(GetVersionParams), 
									&cbBytesReturned, NULL) )
		{
			return false;
		}

		
		// Print the SMART version
        // PrintVersion (& GetVersionParams);
		 
		 // Retrieve the IDENTIFY data, Prepare the command
		RLStream bufferOut(sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE);
        PSENDCMDINPARAMS Command = (PSENDCMDINPARAMS)bufferOut.GetBuffer();
		Command -> irDriveRegs.bCommandReg = 0xEC; // Returns ID sector for ATA
		
		DWORD BytesReturned = 0;
		if ( !::DeviceIoControl (hDrive, SMART_RCV_DRIVE_DATA, Command, sizeof(SENDCMDINPARAMS),
								Command, bufferOut.GetCapacity(), &BytesReturned, NULL) )
		{
			return false;
		} 
		USHORT *pIdSector = (USHORT *)((PSENDCMDOUTPARAMS)bufferOut.GetBuffer()) -> bBuffer; // PIDENTIFY_DATA
		FillHDDInfo(pIdSector, hdd);

		return true;
	}

	static CStringA ConvertToString (UINT16* diskdata, int firstIndex, int lastIndex)
	{
		CStringA out;

		char* buf = out.GetBuffer((lastIndex-firstIndex+1)*2);

		int k = 0;

		//  each integer has two characters stored in it backwards
		for (int index = firstIndex; index <= lastIndex; index++)
		{
			buf [k++] = (char) (diskdata [index] / 256);	//  get high byte for 1st character
			buf [k++] = (char) (diskdata [index] % 256);	//  get low  byte for 2nd character
		}

		out.ReleaseBuffer(k); //  end the string 
		out.TrimRight();
		out.TrimLeft();
		out.MakeUpper();

		return out;
	}

	static void FillHDDInfo (UINT16 diskdata[256], HDD& hdd)
	{	 
		 //  copy the hard drive serial number to the buffer
		hdd.firmwareRev = ConvertToString(diskdata, 23, 26);
		hdd.serial      = ConvertToString(diskdata, 10, 19);		
		hdd.model       = ConvertToString(diskdata, 27, 46);
		//hdd.size = diskdata [21] * 512; // buffer size

	   __int64 sectors;
	   
		//  calculate size based on 28 bit or 48 bit addressing, 48 bit is reflected by bit 10 of word 83
		if (diskdata [83] & 0x400) 
			sectors = diskdata [103] * 65536I64 * 65536I64 * 65536I64 + 
					  diskdata [102] * 65536I64 * 65536I64 + 
					  diskdata [101] * 65536I64 + 
					  diskdata [100];
		else
			sectors = diskdata [61] * 65536 + diskdata [60];

		hdd.size = sectors * 512; //  there are 512 bytes in a sector
	}

	static INT32 GetFirstDiskExtent(INT32 nLogicDrive)
	{
		char devicePath[] = {"\\\\.\\A:"};
		devicePath[4] += (char)nLogicDrive;

		HANDLE hVolume = ::CreateFile(devicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 
								NULL, OPEN_EXISTING, 0, NULL);

		if (hVolume == INVALID_HANDLE_VALUE)
			throw RLException("GetFirstDiskExtent() %u %u", 1, ::GetLastError());

		VOLUME_DISK_EXTENTS DiskExtents;
		DWORD dwBytesWritten = 0;

		BOOL ok = ::DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
									NULL, 0, &DiskExtents, sizeof(DiskExtents), &dwBytesWritten, NULL);

		DWORD error;

		if (!ok) {
			error = ::GetLastError();
			if (error == ERROR_MORE_DATA) ok = TRUE; // not enough for all extends, but have first one, it's OK for us
		}

		::CloseHandle(hVolume);

		if(!ok)
			throw RLException("GetFirstDiskExtent() %u %u", 2, error); //IO_FAILED

		if (DiskExtents.NumberOfDiskExtents==0)
			throw RLException("GetFirstDiskExtent() %u %u", 3, 0);

		DWORD dwDiskNumber = DiskExtents.Extents[0].DiskNumber;	

		if (dwDiskNumber==~0) // may be error in driver
			throw RLException("GetFirstDiskExtent() %u %u", 4, 0);

		return dwDiskNumber;
	}

	
	// primary - true if found HDD with OS
	//
	static HDD GetHDD(bool& primary)
	{
		const int MAX_DISKS = 32;

		HDD  scsi[MAX_DISKS];
		bool scsi_done = false;

		int disk = -1;
		try {
			char szWinDirPath[MAX_PATH];

			if (!SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_SYSTEM, NULL, 0, szWinDirPath)))
				throw RLException("");

			INT32 nLogicDrive = (INT32)::PathGetDriveNumberA(szWinDirPath);
        
			if (nLogicDrive<0 || nLogicDrive>25) // 25 means next after 'Z'  
				throw RLException("");

			disk = GetFirstDiskExtent(nLogicDrive);
		}
		catch(RLException& ) {}

		HDD hdd;

		if (disk>=0) {
			primary = true;
			ReadPhysicalDriveWithZeroRights(disk, hdd);
			if (hdd.IsValid()) return hdd;

			//try to find by SCSI
			if (hdd.size>0) { // and OS < Win7 (vista ???)
				for (int i=0; i<MAX_DISKS; i++) {
					ReadIdeDriveAsScsiDrive(i, scsi[i]);
				}
				scsi_done = true;

				// try to find exact the same size & firmwareRev (model can be a little different)
				int i1 = -1;
				int i2 = -1;
				for (i=0; i<MAX_DISKS; i++) {
					if (scsi[i].IsValid() && scsi[i].size==hdd.size) {

						int c = 1;

						if (scsi[i].firmwareRev==hdd.firmwareRev) c++;
						if (scsi[i].model      ==hdd.model)       c++;

						if (c==3) {
							return scsi[i]; // exact same
						}
						else if (c==2) {
							if (i2<0) i2 = i; // 2/3 paraments the same
						}
						else {
							if (i1<0) i1 = i; // 1/3 paraments the same
						}						
					}
				}

				// found approximate 
				if (i2>=0) return scsi[i2];
				if (i1>=0) return scsi[i1];				
			}

			ReadPhysicalDriveInUsingSmart   (disk, hdd); if (hdd.IsValid()) return hdd;
			ReadPhysicalDriveWithAdminRights(disk, hdd); if (hdd.IsValid()) return hdd;
		}

		// try to find ANY valid disk
		primary = false;
		for (int i=0; i<MAX_DISKS; i++) {
			if (i==disk) continue; // already tried
			ReadPhysicalDriveWithZeroRights(i, hdd);
			if (hdd.IsValid()) return hdd;
		}

		for (i=0; i<MAX_DISKS; i++) {
			if (!scsi_done)
				ReadIdeDriveAsScsiDrive(i, scsi[i]);

			if (scsi[i].IsValid()) return scsi[i];
		}

		for (i=0; i<MAX_DISKS; i++) {
			if (i==disk) continue; // already tried
			ReadPhysicalDriveInUsingSmart(i, hdd);
			if (hdd.IsValid()) return hdd;
		}

		for (i=0; i<MAX_DISKS; i++) {
			if (i==disk) continue; // already tried
			ReadPhysicalDriveWithAdminRights(i, hdd);
			if (hdd.IsValid()) return hdd;
		}

		// return empty HDD
		hdd.Clear();
		return hdd;
	}
};

#endif // RL_HARDWARE_HDD_6341A1__INCLUDED_