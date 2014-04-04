<?php
$login = "smartdraweruser";
$password = "smart";
$databasename = "smartdrawerdb";
$tablename = "person";

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

function getQueryResultsFromPersonTable($dbObjectToQuery, $queryStringToSend) {
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

	return json_encode($returnJsonArray);
}

function respondWithJsonError($errorCode, $errorMessage) {
	$message = array('error'=>true, 'errorCode'=>$errorCode, 'message'=>$errorMessage);
	echo json_encode($message);
	exit();
}

function addUserWithUidFirstnameSurname($uid, $firstname, $surname) {
	$dbObject = connectAndSetDatabase();
	$queryString = "INSERT INTO " . $GLOBALS['tablename'] . " (passid, firstname, surname) VALUES ('" . $uid . "', '" . $firstname . "', '" . $surname . "');";
	$stringToOutput = getQueryResultsFromPersonTable($dbObject, $queryString);
	echo $stringToOutput;
	closeDbConnection($dbObject);
}

function main() {
	$uid = $_GET["uid"];
	$firstname = $_GET["firstname"];
	$surname = $_GET["surname"];

	if ($uid && $firstname && $surname) {
		addUserWithUidFirstnameSurname($uid, $firstname, $surname);
	}
	else {
		respondWithJsonError(0, "URL parameters missing, got uid: " . $uid . ", firstname: ". $firstname . ", surname: " . $surname);
	}
}

//main();

?>
