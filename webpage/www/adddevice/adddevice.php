<?php
$login = "smartdraweruser";
$password = "smart";
$databasename = "smartdrawerdb";
$tablename = "device";

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

function addDeviceWithUidNameDescription($uid, $name, $description) {
	$dbObject = connectAndSetDatabase();
	$queryString = "INSERT INTO " . $GLOBALS['tablename'] . " (passid, name, description) VALUES ('" . $uid . "', '" . $name . "', '" . $description . "');";
	$stringToOutput = getQueryResultsFromPersonTable($dbObject, $queryString);
	echo $stringToOutput;
	closeDbConnection($dbObject);
}

function main() {
	$uid = $_GET["uid"];
	$name = $_GET["name"];
	$description = $_GET["description"];

	if ($uid && $name && $description) {
		addDeviceWithUidNameDescription($uid, $name, $description);
	}
	else {
		respondWithJsonError(0, "URL parameters missing, got uid: " . $uid . ", name: ". $name . ", description: " . $description);
	}
}

//main();

?>
