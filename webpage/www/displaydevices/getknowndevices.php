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

function getQueryString() {
	$parametersSet = $_GET;
	switch ($parametersSet["show"]) {

		case 'raw':
		return "SELECT * FROM ". $GLOBALS['devicetablename'];

		case 'indrawer':
		return "SELECT device.name AS 'devicename', device.description, 'In Smart Drawer' AS 'assignedto' FROM ". $GLOBALS['devicetablename'] . " WHERE userid IS NULL";

		case 'userheld':
		return "SELECT device.name AS 'devicename', device.description, CONCAT(person.firstname, ' ', person.surname) AS 'assignedto' FROM ". $GLOBALS['devicetablename'] . " INNER JOIN " . $GLOBALS['persontablename'] . " ON device.userid=person.passid";

		default:
		return "SELECT device.name AS 'devicename', device.description, 'In Smart Drawer' AS 'assignedto' FROM ". $GLOBALS['devicetablename'] . " WHERE userid IS NULL UNION SELECT device.name AS 'devicename', device.description, CONCAT(person.firstname, ' ', person.surname) AS 'assignedto' FROM ". $GLOBALS['devicetablename'] . " INNER JOIN " . $GLOBALS['persontablename'] . " ON device.userid=person.passid";
		break;
	}
}

function outputDataFromDeviceTable() {
	$dbObject = connectAndSetDatabase();
	echo getQueryResultsFromDeviceTable($dbObject, getQueryString());
	closeDbConnection($dbObject);
}

function getQueryResultsFromDeviceTable($dbObjectToQuery, $queryStringToSend) {
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
	outputDataFromDeviceTable();
}

//main();

?>
