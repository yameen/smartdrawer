<?php
$login = "smartdraweruser";
$password = "smart";
$databasename = "smartdrawerdb";
$persontablename = "person";
$devicetablename = "device";

function connectAndSetDatabase() {
	$mysqli = mysqli_connect("localhost", $GLOBALS['login'], $GLOBALS['password'], $GLOBALS['databasename']);
	$errorCode = mysqli_connect_errno($mysqli);
	if ($errorCode) {
		respondWithJsonError($errorCode, "Failed to connect to MySQL: " . mysqli_connect_error());
	}
	else {
		return $mysqli;
	}
}
function closeDbConnection($dbToClose) {
	if(!$dbToClose->close()) {
		respondWithJsonError($dbToClose->errno, $dbToClose->error);
	}
}

function getQueryResultsForQueryString($dbObjectToQuery, $queryStringToSend) {
	$returnArray = array();
	$result = $dbObjectToQuery->query($queryStringToSend);
	if(!$result) {
		respondWithJsonError($dbObjectToQuery->errno, $dbObjectToQuery->error);
	}
	if($result === TRUE) {
	return TRUE;
	}

	//process the data set returned
	$result->data_seek(0);
	while ($row = $result->fetch_assoc()) {
		array_push($returnArray, $row);
	}

	return $returnArray;
}

function respondWithJsonError($errorCode, $errorMessage, $additionalJsonArray) {
	$message = array('error'=>true, 'errorCode'=>$errorCode, 'message'=>$errorMessage);
	if($additionalJsonArray) {
		$message[additionalData] = $additionalJsonArray;
	}
	echo json_encode($message);
	exit();
}

function assignUserDevice($userid, $deviceid) {
	$dbObject = connectAndSetDatabase();
	$queryString = "SELECT * FROM " . $GLOBALS['persontablename'] . " WHERE passid='" . $userid . "';";
	$personValidResult = getQueryResultsForQueryString($dbObject, $queryString);

	if(count($personValidResult) == 1) {
		//the userid is valid, now check that the device exists
		$queryString = "SELECT * FROM " . $GLOBALS['devicetablename'] . " WHERE passid='" . $deviceid . "';";
		$deviceValidResult = getQueryResultsForQueryString($dbObject, $queryString);

		if(count($deviceValidResult) == 1) {
			//device is valid, now perform assignment
			$updateQueryString = "UPDATE " . $GLOBALS['devicetablename'] . " SET userid='" . $userid . "' WHERE passid='" . $deviceid . "';";
			$updateQueryResult = getQueryResultsForQueryString($dbObject, $updateQueryString);
			if($updateQueryResult != TRUE) {
				respondWithJsonError(2, "Attempted update of table ". $GLOBALS['devicetablename'] . "did not complete successfuly");
			}
			else {
				$message = array('error'=>false, 'message'=>"Assigned device with id " . $deviceid . " to user with id " . $userid);
				echo json_encode($message);
			}
		}
		else {
			respondWithJsonError(3, "Device with id " . $deviceid . " was not found or has multiple entries ", $deviceValidResult);
		}

	}
	else {
			respondWithJsonError(3, "User with id " . $userid . " was not found or has multiple entries ", $personValidResult);
		}

	closeDbConnection($dbObject);
}

function main() {
	$userid = $_GET["userid"];
	$deviceid = $_GET["deviceid"];


	if ($userid && $deviceid) {
		assignUserDevice($userid, $deviceid);
	}
	else {
		respondWithJsonError(0, "URL parameters missing, got userid: " . $userid . ", deviceid: ". $deviceid);
	}
}

//main();

?>
