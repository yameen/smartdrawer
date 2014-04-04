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

function unassignUserDevice($deviceid) {
	$dbObject = connectAndSetDatabase();

		$queryString = "SELECT * FROM " . $GLOBALS['devicetablename'] . " WHERE passid='" . $deviceid . "';";
		$deviceValidResult = getQueryResultsForQueryString($dbObject, $queryString);

		if(count($deviceValidResult) == 1) {
			//device is valid, now perform assignment
			$updateQueryString = "UPDATE " . $GLOBALS['devicetablename'] . " SET userid=NULL WHERE passid='" . $deviceid . "';";
			$updateQueryResult = getQueryResultsForQueryString($dbObject, $updateQueryString);
			if($updateQueryResult != TRUE) {
				respondWithJsonError(2, "Attempted update of table ". $GLOBALS['devicetablename'] . "did not complete successfuly");
			}
			else {
				$message = array('error'=>false, 'message'=>"Unassigned device with id " . $deviceid);
				echo json_encode($message);
			}
		}
		else {
			respondWithJsonError(3, "Device with id " . $deviceid . " was not found or has multiple entries ", $deviceValidResult);
		}

	closeDbConnection($dbObject);
}

function main() {
	$deviceid = $_GET["deviceid"];

	if ($deviceid) {
		unassignUserDevice($deviceid);
	}
	else {
		respondWithJsonError(0, "URL parameters missing, got deviceid: ". $deviceid);
	}
}

//main();

?>
