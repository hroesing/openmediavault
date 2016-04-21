<?php
/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2016 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenMediaVault. If not, see <http://www.gnu.org/licenses/>.
 */
namespace OMV\System\Storage;

/**
 * This class implements methods to get and process S.M.A.R.T.
 * AT Attachment (ATA) information and properties.
 * @ingroup api
 */
class SmartInformation {
	protected $deviceFile = "";
	protected $type = "";
	protected $sd = NULL;
	private $cmdOutput = NULL;
	private $dataCached = FALSE;
	private $attrDesc = array(
		1 => "Frequency of errors while reading raw data from the disk",
		2 => "Average efficiency of the disk",
		3 => "Time needed to spin up the disk",
		4 => "Number of spindle start/stop cycles",
		5 => "Number of remapped sectors",
		6 => "Margin of a channel while reading data",
		7 => "Frequency of errors while positioning",
		8 => "Average efficiency of operations while positioning",
		9 => "Number of hours elapsed in the power-on state",
		10 => "Number of retry attempts to spin up",
		11 => "Number of attempts to calibrate the device",
		12 => "Number of power-on events",
		13 => "Frequency of errors while reading from the disk",
		173 => "Counts the maximum worst erase count on any block",
		187 => "Number of errors that could not be recovered using hardware ECC",
		188 => "The count of aborted operations due to HDD timeout",
		189 => "Number of times a recording head is flying outside its normal operating range",
		190 => "Airflow temperature of the drive",
		191 => "Frequency of mistakes as a result of impact loads",
		192 => "Number of power-off or emergency retract cycles",
		193 => "Number of cycles into landing zone position",
		194 => "Current internal temperature of the drive",
		195 => "Number of ECC on-the-fly errors",
		196 => "Number of remapping operations",
		197 => "Number of sectors waiting to be remapped",
		198 => "The total number of uncorrectable errors when reading/writing a sector",
		199 => "Number of CRC errors during UDMA mode",
		200 => "Number of errors found when writing a sector",
		201 => "Number of off-track errors",
		203 => "Number of ECC errors",
		204 => "Number of errors corrected by software ECC",
		205 => "Number of errors due to high temperature",
		206 => "Height of heads above the disk surface",
		207 => "Amount of high current used to spin up the drive",
		208 => "Number of buzz routines to spin up the drive",
		209 => "Drive’s seek performance during offline operations",
		220 => "Distance the disk has shifted relative to the spindle",
		221 => "Number of errors as a result of impact loads as detected by a shock sensor",
		222 => "Number of hours in general operational state",
		223 => "Number of times head changes position",
		224 => "Load on drive caused by friction in mechanical parts of the store",
		225 => "Total number of load cycles",
		226 => "General time for loading in a drive",
		227 => "Number of attempts to compensate for platter speed variations",
		228 => "Number of power-off retract events",
		230 => "Amplitude of heads trembling in running mode",
		231 => "Temperature of the drive",
		232 => "Number of physical erase cycles completed on the drive as a percentage of the maximum physical erase cycles the drive supports",
		233 => "Number of hours elapsed in the power-on state",
		235 => "Number of available reserved blocks as a percentage of the total number of reserved blocks",
		240 => "Time spent during the positioning of the drive heads",
		250 => "Number of errors while reading from a disk",
		251 => "Number of remaining spare blocks as a percentage of the total number of spare blocks available",
		252 => "Total number of bad flash blocks the drive detected since it was first initialized in manufacturing",
		254 => "Number of detected free fall events"
	);

	const SMART_ASSESSMENT_GOOD = "GOOD";
	const SMART_ASSESSMENT_BAD_ATTRIBUTE_IN_THE_PAST = "BAD_ATTRIBUTE_IN_THE_PAST";
	const SMART_ASSESSMENT_BAD_SECTOR = "BAD_SECTOR";
	const SMART_ASSESSMENT_BAD_ATTRIBUTE_NOW = "BAD_ATTRIBUTE_NOW";
	const SMART_ASSESSMENT_BAD_SECTOR_MANY = "BAD_SECTOR_MANY";
	const SMART_ASSESSMENT_BAD_STATUS = "BAD_STATUS";

	/**
	 * Constructor
	 * @param deviceFile Specifies the device file, e.g.
	 *   <ul>
	 *   \li /dev/sdb
	 *   \li /dev/cciss/c0d0
	 *   \li /dev/disk/by-id/scsi-SATA_ST3200XXXX2AS_5XWXXXR6
	 *   \li /dev/disk/by-path/pci-0000:00:10.0-scsi-0:0:0:0
	 *   </ul>
	 * @param type Specifies the type of the device, e.g. 'ata', 'cciss'
	 *   or '3ware'. For more details please check the option '-d' in
	 *   http://smartmontools.sourceforge.net/man5/smartd.conf.5.html.
	 * @param sd The storage device object. Defaults to NULL.
	 */
	public function __construct($deviceFile, $type, $sd = null) {
		$this->deviceFile = $deviceFile;
		$this->type = $type;
		$this->sd = $sd;
	}

	/**
	 * Get the device path, e.g. /dev/sda.
	 * @return The device path.
	 */
	public function getDeviceFile() {
		return $this->deviceFile;
	}

	/**
	 * Get the type of the device, e.g. 'ata', 'sat', 'cciss' or '3ware'.
	 * Note, this can be an empty string.
	 * @return The device type.
	 */
	public function getType() {
		return $this->type;
	}

	/**
	 * Get S.M.A.R.T. information.
	 * @private
	 * @return None.
	 * @throw \OMV\ExecException
	 */
	private function getData() {
		if ($this->dataCached !== FALSE)
			return;

		// Get all available S.M.A.R.T. information.
		$cmdArgs = array();
		$cmdArgs[] = sprintf("-x %s", escapeshellarg($this->deviceFile));
		if (!empty($this->type))
			$cmdArgs[] = sprintf("-d %s", $this->type);
		$cmd = new \OMV\System\Process("smartctl", $cmdArgs);
		$cmd->setRedirect2to1(TRUE);
		$cmd->setQuiet(TRUE);
		$cmd->execute($output, $exitCode);
		// Bit 0: Command line did not parse
		// Bit 1: Device open failed, or device did not return an
		//        IDENTIFY DEVICE structure
		if (($exitCode !== 0) && (($exitCode & 0) || ($exitCode & 1)))
			throw new \OMV\ExecException($cmd->getCommandLine(), $output);
		$this->cmdOutput = $output;

		// Set flag to mark information has been successfully read.
		$this->dataCached = TRUE;
	}

	/**
	 * Refresh the cached information.
	 * @return None.
	 */
	public function refresh() {
		$this->dataCached = FALSE;
		$this->getData();
	}

	/**
	 * Get the plain text 'smartctl' command output.
	 * @return The command output.
	 */
	public function getExtendedInformation() {
		$this->getData();
		return implode("\n", $this->cmdOutput);
	}

	/**
	 * Get the S.M.A.R.T. attributes data structure.
	 * @return An array of S.M.A.R.T. attributes data.
	 */
	public function getAttributes() {
		$this->getData();
		$result = array();
		foreach ($this->cmdOutput as $cmdOutputk => $cmdOutputv) {
			// smartctl 5.41 2011-06-09 r3365
			// SMART Attributes Data Structure revision number: 16
			// Vendor Specific SMART Attributes with Thresholds:
			// ID# ATTRIBUTE_NAME          FLAGS    VALUE WORST THRESH FAIL RAW_VALUE
			//   1 Raw_Read_Error_Rate     POSR-K   100   100   051    -    48
			//   2 Throughput_Performance  -OS--K   055   055   000    -    18907
			//   3 Spin_Up_Time            PO---K   068   068   025    -    9773
			//   4 Start_Stop_Count        -O--CK   100   100   000    -    795
			//   5 Reallocated_Sector_Ct   PO--CK   252   252   010    -    0
			//   7 Seek_Error_Rate         -OSR-K   252   252   051    -    0
			$regex = '/^\s*(\d+)\s+(\S+)\s+([POSRCK-]+)\s+(\d+)\s+(\d+)\s+'.
			  '(\d+|---)\s+(\S+)\s+(.+)$/i';
			if (1 == preg_match($regex, $cmdOutputv, $matches)) {
				$id = intval($matches[1]);
				$attrData = array(
					"id" => $id,
					"attrname" => $matches[2],
					"flags" => $matches[3],
					"value" => intval($matches[4]),
					"worst" => intval($matches[5]),
					"treshold" => ("---" == $matches[6]) ?
					  0 : intval($matches[6]),
					"whenfailed" => $matches[7],
					"rawvalue" => $matches[8],
					"description" => array_key_exists($id, $this->attrDesc) ?
					  $this->attrDesc[$id] : "",
					// Additional fields used for the assessment.
					"prefailure" => (FALSE !== stripos($matches[3], "P")),
					"assessment" => self::SMART_ASSESSMENT_BAD_STATUS
				);
				// Assess prefailure attributes.
				if (TRUE === $attrData['prefailure'])
					$this->assessPrefailureAttribute($attrData);
				$result[] = $attrData;
			}
		}
		return $result;
	}

	private function assessPrefailureAttribute(&$attrData) {
		if (FALSE === $attrData['prefailure'])
			return;
		// Set the default assessment.
		$attrData['assessment'] = self::SMART_ASSESSMENT_GOOD;
		// Always-Fail and Always-Passing thresholds are not relevant
		// for our assessment.
		if (!(1 <= $attrData['treshold']) && (0xFD >= $attrData['treshold'])) {
			$attrData['assessment'] = self::SMART_ASSESSMENT_BAD_STATUS;
			return;
		}
		// See https://wiki.ubuntuusers.de/Festplattenstatus
		if ($attrData['value'] <= $attrData['treshold']) {
			$attrData['assessment'] = self::SMART_ASSESSMENT_BAD_ATTRIBUTE_NOW;
			return;
		}
		if ($attrData['worst'] <= $attrData['treshold']) {
			$attrData['assessment'] = self::SMART_ASSESSMENT_BAD_ATTRIBUTE_IN_THE_PAST;
			return;
		}
	}

	/**
	 * Get a specific attribute by ID.
	 * @return An object with the requested attribute data, otherwise FALSE.
	 */
	public function getAttribute($id) {
		if (FALSE === ($attributes = $this->getAttributes()))
			return FALSE;
		$result = FALSE;
		foreach ($attributes as $attrk => $attrv) {
			if ($attrv['id'] == $id) {
				$result = $attrv;
				break;
			}
		}
		return $result;
	}

	/**
	 * Get the S.M.A.R.T. Self-test log structure.
	 * @return An array of S.M.A.R.T. self-test logs.
	 */
	public function getSelfTestLogs() {
		$this->getData();
		$result = array();
		foreach ($this->cmdOutput as $cmdOutputk => $cmdOutputv) {
			// SMART Self-test log structure
			// Parse command output:
			// Num  Test_Description    Status                  Remaining  LifeTime(hours)  LBA_of_first_error
			// # 1  Extended offline    Completed: read failure       90%       670         57217755
			// # 2  Short captive       Interrupted (host reset)      80%      1392         -
			$regex = '/^#\s*(\d+)\s+(Short offline|Extended offline|'.
			  'Short captive|Extended captive)\s+(.+)\s+(\d+)%\s+(\d+)'.
			  '\s+(.+)$/';
			if (1 == preg_match($regex, $cmdOutputv, $matches)) {
				$result[] = array(
					"num" => $matches[1],
					"description" => $matches[2],
					"status" => $matches[3],
					"remaining" => $matches[4],
					"lifetime" => $matches[5],
					"lbaoffirsterror" => $matches[6]
				);
			}
		}
		return $result;
	}

	/**
	 * Get various device information.
	 * @return An array of strings.
	 */
	public function getInformation() {
		$this->getData();
		// Initialize with default values. Note, the result list may
		// contain additional key/value pairs.
		$result = array(
			"devicemodel" => "",
			"serialnumber" => "",
			"firmwareversion" => "",
			"usercapacity" => ""
		);
		// INFORMATION SECTION
		// Parse command output:
		// === START OF INFORMATION SECTION ===
		// Model Family:     Western Digital RE3 Serial ATA family
		// Device Model:     WDC WD2502ABYS-02B7A0
		// Serial Number:    WD-WCAV1B245569
		// Firmware Version: 02.03B03
		// User Capacity:    251,059,544,064 bytes
		// Device is:        In smartctl database [for details use: -P show]
		// ATA Version is:   8
		// ATA Standard is:  Exact ATA specification draft version not indicated
		// Local Time is:    Tue Mar 11 10:18:42 2014 CET
		// SMART support is: Available - device has SMART capability.
		// SMART support is: Enabled
		//
		// === START OF READ SMART DATA SECTION ===
		// ...
		$sectionFound = FALSE;
		foreach ($this->cmdOutput as $cmdOutputk => $cmdOutputv) {
			$cmdOutputv = trim($cmdOutputv);
			// Abort parsing, we are not interested in the information
			// shown below this line.
			if ($cmdOutputv == "=== START OF READ SMART DATA SECTION ===")
				break;
			// Ignore everything that is not below this line.
			if ($cmdOutputv == "=== START OF INFORMATION SECTION ===") {
				$sectionFound = TRUE;
				continue;
			}
			// Have we found the information section?
			if (FALSE === $sectionFound)
				continue;
			// Parse the information section line:
			// Device Model:     WDC WD2502ABYS-02B7A0
			$regex = '/^([^:]+):\s+(.+)$/i';
			if (1 !== preg_match($regex, $cmdOutputv, $matches))
				continue;
			// Convert the attribute name, e.g. 'Device Model' to
			// 'devicemodel'.
			$attrKey = strtolower(str_replace(" ", "", $matches[1]));
			// Append key/value to result array.
			$result[$attrKey] = $matches[2];
		}
		return $result;
	}

	/**
	 * Get the device temperature in °C (value only, no unit).
	 * @return The temperature value, otherwise FALSE.
	 */
	public function getTemperature() {
		if (FALSE === ($attributes = $this->getAttributes()))
			return FALSE;
		$result = 0;
		$found = FALSE;
		// Process the attributes to get the temperature value.
		foreach ($attributes as $attrk => $attrv) {
			switch ($attrv['id']) {
			case 190:
				// ID# ATTRIBUTE_NAME          FLAG     VALUE WORST THRESH TYPE      UPDATED  WHEN_FAILED RAW_VALUE
				// 190 Airflow_Temperature_Cel 0x0022   040   039   045    Old_age   Always   FAILING_NOW 60 (0 209 61 41)
				//
				// ID# ATTRIBUTE_NAME          FLAGS    VALUE WORST THRESH FAIL RAW_VALUE
				// 190 Airflow_Temperature_Cel -O---K   065   044   045    Past 35 (0 3 35 35 0)
			case 194:
				// ID# ATTRIBUTE_NAME          FLAG     VALUE WORST THRESH TYPE      UPDATED  WHEN_FAILED RAW_VALUE
				// 194 Temperature_Celsius     0x0002   214   214   000    Old_age   Always       -       28 (Lifetime Min/Max 21/32)
				// 194 Temperature_Celsius     0x0022   060   061   000    Old_age   Always       -       60 (0 20 0 0)
				// 194 Temperature_Celsius     0x0022   030   055   000    Old_age   Always       -       30 (Min/Max 17/55)
				//
				// ID# ATTRIBUTE_NAME          FLAGS    VALUE WORST THRESH FAIL RAW_VALUE
				// 194 Temperature_Celsius     -O---K   076   037   ---    -    24 (Min/Max 19/37)
			case 231: // temperature-celsius
				// ID# ATTRIBUTE_NAME          FLAG     VALUE WORST THRESH TYPE      UPDATED  WHEN_FAILED RAW_VALUE
				// 231 Temperature_Celsius     0x0013   100   100   010    Pre-fail  Always       -       0
				$regex = '/^(\d+)(\s+(.+))*$/';
				if (1 !== preg_match($regex, $attrv['rawvalue'], $matches))
					continue;
				// Verify temperature.
				$temp = intval($matches[1]);
				if ((-15 > $temp) || (100 < $temp))
					continue;
				if (!$found || ($temp > $result))
					$result = $temp;
				$found = TRUE;
				break;
			}
			if (TRUE === $found)
				break;
		}
		// If the SMART attributes are not present then it may be an older
		// SCSI device. Then the command output looks like:
		// Device: SEAGATE  ST336605LSUN36G  Version: 0238
		// Serial number: 3FP1J35V00007241EEC7
		// Device type: disk
		// Local Time is: Thu Apr  5 15:41:56 2012 CEST
		// Device supports SMART and is Enabled
		// Temperature Warning Enabled
		// SMART Health Status: OK
		//
		// Current Drive Temperature:     34 C
		// Drive Trip Temperature:        65 C
		// ...
		//
		// Alternative output:
		//
		// ...
		// SCT Status Version:                  3
		// SCT Version (vendor specific):       256 (0x0100)
		// SCT Support Level:                   1
		// Device State:                        DST executing in background (3)
		// Current Temperature:                    28 Celsius
		// Power Cycle Min/Max Temperature:     23/28 Celsius
		// Lifetime    Min/Max Temperature:     21/32 Celsius
		// ...
		if (FALSE === $found) {
			foreach ($this->cmdOutput as $cmdOutputk => $cmdOutputv) {
				$regex = '/^(Current Drive Temperature|Current Temperature):'.
				  '\s+(\d+)\s+(C|Celsius)$/i';
				if (1 == preg_match($regex, $cmdOutputv, $matches)) {
					$result = $matches[2];
					$found = TRUE;
					break;
				}
			}
		}
		return (TRUE === $found) ? $result : FALSE;
	}

	/**
	 * Get the overall assessment for the device.
	 * @return Returns the following strings:
	 *    <ul>
	 *   \li GOOD
	 *   \li BAD_ATTRIBUTE_NOW
	 *   \li BAD_ATTRIBUTE_IN_THE_PAST
	 *   \li BAD_SECTOR
	 *   \li BAD_SECTOR_MANY
	 *   \li BAD_STATUS
	 *   </ul>
	 *   or otherwise FALSE.
	 */
	public function getOverallStatus() {
		if (FALSE === ($attributes = $this->getAttributes()))
			return FALSE;
		if (empty($attributes))
			return self::SMART_ASSESSMENT_BAD_STATUS;
		// Checks are adapted from libatasmart.
		// See http://git.0pointer.net/libatasmart.git
		// Get number of bad sectors.
		$numSectors = 0;
		$attrData = $this->getAttribute(5); // Reallocated_Sector_Ct
		if (is_array($attrData))
			$numSectors += $attrData['rawvalue'];
		$attrData = $this->getAttribute(197); // Current_Pending_Sector
		if (is_array($attrData))
			$numSectors += $attrData['rawvalue'];
		// Check if the number of bad sectors is greater than a
		// certain threshold.
		// !!! Note, currently this check is only available on 64bit systems
		// because i'm too lazy to implement the log function using the BC
		// math library to support this check on 32bit systems (the getSize()
		// method may return really big integers for storage devices nowadays
		// which can't be handled on 32bit systems anymore).
		if (is_64bits() && !is_null($this->sd)) {
			// Get the size of the device in bytes.
			$size = intval($this->sd->getSize());
			$sectorThreshold = intval(log($size / 512) * 1024);
			if ($numSectors >= $sectorThreshold)
				return self::SMART_ASSESSMENT_BAD_SECTOR_MANY;
		}
		// Check if any of the SMART attributes is bad.
		foreach ($attributes as $attrk => $attrv) {
			// Skip non-prefailure attributes.
			if (FALSE === $attrv['prefailure'])
				continue;
			// Skip attributes that do not have a valid assessment.
			if (self::SMART_ASSESSMENT_BAD_STATUS == $attrv['assessment'])
				continue;
			if (self::SMART_ASSESSMENT_GOOD !== $attrv['assessment'])
				return $attrv['assessment'];
		}
		// Check if there are any bad sectors at all.
		if ($numSectors > 0)
			return self::SMART_ASSESSMENT_BAD_SECTOR;
		// There's really nothing to complain about, so give it a pass.
		return self::SMART_ASSESSMENT_GOOD;
	}
}