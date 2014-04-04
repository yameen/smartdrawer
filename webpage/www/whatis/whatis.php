<?php
$login = "smartdraweruser";
$password = "smart";
$databasename = "smartdrawerdb";
$devicetablename = "device";
$persontablename = "person";

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

function getQueryResults($dbObjectToQuery, $queryStringToSend) {
	$returnJsonArray = array();
	$result = $dbObjectToQuery->query($queryStringToSend);
	if(!$result) {
		respondWithJsonError($dbObjectToQuery->errno, $dbObjectToQuery->error);
	}
	if($result === TRUE) {
	$message = array('error'=>false, 'message'=>"successful execution of SQL: " . $queryStringToSend);
	return json_encode($message);
	}

	//process the data set returned
	$result->data_seek(0);
	while ($row = $result->fetch_assoc()) {
		array_push($returnJsonArray, $row);
	}

	return $returnJsonArray;
}

function respondWithJsonError($errorCode, $errorMessage, $additionalJsonArray) {
	$message = array('error'=>true, 'errorCode'=>$errorCode, 'message'=>$errorMessage);
	if($additionalJsonArray) {
		$message[additionalData] = $additionalJsonArray;
	}
	echo json_encode($message);
	exit();
}

function checkUidAgainstKnownPersons($uid, $dbObject) {
	$queryString = "SELECT * FROM " . $GLOBALS['persontablename'] . " WHERE passid='" . $uid . "';";
	$arrayToOutput = getQueryResults($dbObject, $queryString);
	return $arrayToOutput;
}
function checkUidAgainstKnownDevices($uid, $dbObject) {
	$queryString = "SELECT * FROM " . $GLOBALS['devicetablename'] . " WHERE passid='" . $uid . "';";
	$arrayToOutput = getQueryResults($dbObject, $queryString);
	return $arrayToOutput;
}
function countNumberOfTotalResults($personResultsArray, $deviceResultArray) {
	$totalNumberOfResults = count($personResultsArray) + count ($deviceResultArray);
	return $totalNumberOfResults;
}
function getSingleReturnedElementFromTwoArrays($personResultsArray, $deviceResultArray) {
	$resultArrayToUse = count($personResultsArray) == 1 ? $personResultsArray : $deviceResultArray;
	return $resultArrayToUse[0];
}
function getTypeOfElementFromTwoArrays($personResultsArray, $deviceResultArray) {

	$arrayWithResultIn = getSingleReturnedElementFromTwoArrays($personResultsArray, $deviceResultArray);
	$typeOfFoundElement = count($personResultsArray) == 1 ? 'person' : 'device';

	//check if the device / person is special add device or add user
	if($typeOfFoundElement == 'person') {
		if($arrayWithResultIn['firstname'] == 'add' && $arrayWithResultIn['surname'] == 'user')
			$typeOfFoundElement = 'addPersonCard';
	}
	if ($typeOfFoundElement == 'device') {
		if($arrayWithResultIn['name'] == 'add' && $arrayWithResultIn['description'] == 'device')
			$typeOfFoundElement = 'addDeviceCard';
	}

	return $typeOfFoundElement;
}

function main() {
	$uid = $_GET["uid"];
	if ($uid) {
		//open db, pass handle to check methods
		$dbObject = connectAndSetDatabase();
		
		$queryResultPerson = checkUidAgainstKnownPersons($uid, $dbObject);
		$queryResultDevice = checkUidAgainstKnownDevices($uid, $dbObject);
		
		closeDbConnection($dbObject);

		$numberOfResults = countNumberOfTotalResults($queryResultPerson, $queryResultDevice);

		if($numberOfResults > 1) {
			//there is more than one entry with this UID, send error response
			respondWithJsonError(1, "Found more than one instance of uid ", array('personResult' => $queryResultPerson, 'deviceResult' => $queryResultDevice));
		}

		if($numberOfResults == 0) {
			//this uid is unique send appropriate response
			$message = array('error' => false, 'message' => "uid is unique", 'type'=>'unknown', 'uid' => $uid);
			echo json_encode($message);
		}

		if($numberOfResults == 1) {
			//this is already present, return the type and details
			$typeOfElement = getTypeOfElementFromTwoArrays ($queryResultPerson, $queryResultDevice);
			$message = array('error' => false, 'message' => "uid exists", 'type' => $typeOfElement, 'uid' => $uid, 'element' => getSingleReturnedElementFromTwoArrays($queryResultPerson, $queryResultDevice));
			echo json_encode($message);
		}
		//echo "\nPERSON " . json_encode($queryResultPerson) . " DEVICE: " . json_encode($queryResultDevice);

	}
	else {
		respondWithJsonError(0, "URL parameters missing, got uid: " . $uid);
	}
}

//main();

?>
