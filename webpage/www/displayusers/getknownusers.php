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

function getQueryString($show) {
	if(!$show) {
		$parametersSet = $_GET;
	}
	else {
		$parametersSet = array("show"=>$show);
	}

	switch ($parametersSet["show"]) {
		case 'names':
		return "SELECT firstname, surname FROM ". $GLOBALS['tablename'];
		break;

		case 'ids':
		return "SELECT passid FROM ". $GLOBALS['tablename'];

		default:
		return "SELECT * FROM ". $GLOBALS['tablename'];
		break;
	}
}

function outputDataFromPersonTable($show) {
	$dbObject = connectAndSetDatabase();
	echo getQueryResultsFromPersonTable($dbObject, getQueryString($show));
	closeDbConnection($dbObject);
}

function getQueryResultsFromPersonTable($dbObjectToQuery, $queryStringToSend) {
	$returnJsonArray = array();
	$result = $dbObjectToQuery->query($queryStringToSend);
	if(!$result) {
		respondWithJsonError($dbObjectToQuery->errno, $dbObjectToQuery->error);
	}

	$result->data_seek(0);
	while ($row = $result->fetch_assoc()) {
		array_push($returnJsonArray, $row);
	}

	return json_encode($returnJsonArray);
}

function respondWithJsonError($errorCode, $errorMessage) {
	$message = array('error'=>true, 'errorCode'=>$errorCode, 'errorMessage'=>$errorMessage);
	echo json_encode($message);
	exit();
}

function main() {
	outputDataFromPersonTable();
}

//main();

?>
