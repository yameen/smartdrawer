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


function getDetailsForUid($uid, $dbObject){
	
	$queryToExecute = "SELECT * FROM " . $GLOBALS['persontablename'] . " WHERE passid = " . $uid . ";";
	$outputArray = getQueryResults($dbObject, $queryToExecute);

	if(count($outputArray) == 0) {
		respondWithJsonError(1, "No user with uid: " . $uid);
	}
	else {
		return $outputArray;
	}
	
}

function getAllDevicesForUserWithUid($uid, $userDetails) {
	$dbObject = connectAndSetDatabase();
	if(!$userDetails) {
		$userDetails = getDetailsForUid($uid, $dbObject);
	}
	$queryToExecute = "SELECT * FROM " . $GLOBALS['devicetablename'] . " WHERE userid = " . $uid . ";";
	$outputArray = getQueryResults($dbObject, $queryToExecute);
	closeDbConnection($dbObject);

	if($outputArray === TRUE) {
	}
	else {
		if(count($outputArray) == 0) {
			$message = array('error'=>false, 'message'=>"uid: " . $uid . "(user, firstname: " . $userDetails[0]['firstname'] . " surname: " . $userDetails[0]['surname'] . ") has no devices assigned.");
			echo json_encode($message);
		}
		else {
		echo json_encode($outputArray);
	}

	}

	exit();
}

function getAllDevicesForUserWithFirstnameSurname($firstname, $surname) {
	$dbObject = connectAndSetDatabase();

	$queryToExecute = "SELECT * FROM " . $GLOBALS['persontablename'] . " WHERE firstname = '" . $firstname . "' AND surname = '" . $surname . "';";
	$outputArray = getQueryResults($dbObject, $queryToExecute);
	closeDbConnection($dbObject);
	
	if(count($outputArray) == 0) {
		respondWithJsonError(1, "User with firstname: " . $firstname . " and surname: " . $surname . " not found in database");
	}
	else {
		$userDetails = array();
		$innerArray = array('firstname' => $firstname, 'surname' => $surname);
		array_push($userDetails, $innerArray);
		getAllDevicesForUserWithUid($outputArray[0]['passid'], $userDetails);
	}
}

function main() {
	$uid = $_GET["uid"];
	$firstname = $_GET["firstname"];
	$surname = $_GET["surname"];
	if ($uid) {
		if($firstname||$surname) {
			respondWithJsonError(0, "You have supplied incorrect parameters, only supply uid OR both firstname and surname");
		}
		else {
			//have uid
			getAllDevicesForUserWithUid($uid);
		}
	}
	else {
		if($firstname && $surname) {
			// have firstname and surname
			getAllDevicesForUserWithFirstnameSurname($firstname, $surname);
		}
		else {
			respondWithJsonError(0, "URL parameters incorrect, no uid and missing parameter, got firstname: " . $firstname . " surname: " . $surname . " both firstname and surname must be present OR just the uid");			
		}
		
	}
}

//main();

?>
