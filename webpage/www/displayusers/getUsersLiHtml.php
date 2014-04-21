<?php

//Simple PHP to create <li> elements for all known users

include 'getknownusers.php';

function run() {
	$dbObject = connectAndSetDatabase();
	$arrayOfUsers = json_decode(getQueryResultsFromPersonTable($dbObject, getQueryString($show)), true);
	closeDbConnection($dbObject);

	for ($i=0; $i < count($arrayOfUsers); $i++) {
		echo '<li><a href="devicesforuser/alldevicesforuser.html?uid='.$arrayOfUsers[$i]['passid'] .'&name='.$arrayOfUsers[$i]['firstname'] . '+'. $arrayOfUsers[$i]['surname'].'">' . $arrayOfUsers[$i]['firstname'] . ' '. $arrayOfUsers[$i]['surname'] .'</a></li>';
	}
}

run();

?>
