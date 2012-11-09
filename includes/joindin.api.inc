<?php

/**
 * @file
 * API calls for the joind.in service
 *
 * This file contains wrapper functions for all calls to joind.in
 * This could be for
 *  all events (paged)      /events
 *  specific event          /events/<id>
 *  all talks at an event   /events/<id>/talks
 *  a specific talk         /talks/<talk id>
 *  comments for a talk     /talks/<talk id>/comments
 *  specific comment        /talks/[talk_id]/comments/[comment_id]
 *  event comments          /events/[event_id]/comments
 *  specific comment        /events/[event_id]/comments/[comment_id]
 *  get a user              /users/[user_id]
 *  get a list of all events a specific user has attended
 *                          /users/[user_id]/attended
 *  get a list of all talks a specific user has given
 *                          /users/[user_id]/talks
 */

/**
 * Calls the joindin api at the given path
 *
 * This function does take heed of the joindin_base_url and joindin_api_version
 * Drupal variables and so should be used in most cases to call the API

 * @param string $path path of the api call after the API version
 *
 * @return mixed|null
 * array containing joindin response, or null if no successful response received
 */
function joindin_call_service($path) {
  // construct uri
  $url = variable_get('joindin_base_url');
  $api_version = variable_get('joindin_api_version');
  return joindin_call_service_by_uri("$url/$api_version/$path");
}

/**
 * Makes the joindin api call specified by the $uri
 *
 * This function does not take heed of the joindin_base_url and
 * joindin_api_version Drupal variables and so should not be used directly
 * unless loading a URI returned directly by a joindin api response
 *
 * @param string $uri full URI to the joindin location
 * @return mixed|null result of the API call if successful, otherwise NULL
 */
function joindin_call_service_by_uri($uri) {
  // call service
  $result = NULL;
  $http_response = drupal_http_request($uri);
  // if our call was successful..
  if ($http_response->code == 200) {
    // ..decode and return the result
    $result = drupal_json_decode($http_response->data);
  }
  else {
    $variables = array(
      '%uri' => $uri,
      '%code' => $http_response->code,
    );
    watchdog('joindin', "Call to joind.in URI %uri failed with return code %code", $variables);
  }
  return $result;
}

/**
 * Retrieves event from joind.in
 *
 * @param int $ji_event_id event id on joind.in
 *
 * @return null|object
 * object representing event, or null if no reasonable response returned
 */

function joindin_get_ji_event($ji_event_id) {
  $result = NULL;
  // ask joindin for the event..
  $response = joindin_call_service("events/$ji_event_id");
  //..if we received a response, containing an event..
  if ($response
    && array_key_exists('events', $response)
    && array_key_exists(0, $response['events'])
    && array_key_exists('name', $response['events'][0])) {
    //..return just that one event
    $result = (object)$response['events'][0];
  }
  else {
    $variables = array(
      '%ji_event_id' => $ji_event_id,
    );
    watchdog('joindin', 'No valid data received for event %ji_event_id', $variables);
  }

  return $result;
}

/**
 * Gets the talk events from joind.in for the specified event
 *
 * @param int $ji_event_id event id on joind.in
 * @return array|null listing talks
 */
function joindin_get_ji_event_talks($ji_event_id) {
  $result = NULL;
  // ask joindin for the talks for the specified event...
  $response = joindin_call_service("events/$ji_event_id/talks");
  //..if we received a response, containing an array of talks..
  if ($response
    && array_key_exists('talks', $response)
    && is_array($response['talks'])) {
    //..return those talks
    $result = $response['talks'];
  }
  else {
    $variables = array(
      '%ji_event_id' => $ji_event_id,
    );
    watchdog('joindin', 'No valid talks data received for event %ji_event_id', $variables);
  }
  return $result;
}

/**
 * @param int $ji_event_id event id on joind.in
 * @return mixed|null array listing comments for the given event
 */
function joindin_get_ji_event_comments($ji_event_id) {
  $result = NULL;
  $response = joindin_call_service("events/$ji_event_id/comments");
  if (array_key_exists('comments', $response) && is_array($response['comments'])) {
    $result = $response['comments'];
  }
  else {
    $variables = array(
      '%ji_event_id' => $ji_event_id,
    );
    watchdog('joindin', 'No valid commments data received for event %ji_event_id', $variables);
  }
  return $result;
}

/**
 * @param int $ji_talk_id talk id on joind.in
 * @return mixed|null object representing the talk
 */
function joindin_get_ji_talk($ji_talk_id) {
  return joindin_call_service("talks/$ji_talk_id");
}

/**
 * @param int $ji_talk_id talk id on joind.in
 * @return mixed|null object representing the talk
 */
function joindin_get_ji_talk_comments($ji_talk_id) {
  return joindin_call_service("talks/$ji_talk_id/comments");
}

/**
 * @param string $comments_uri full uri to get a talks comments via the api
 * @return array|null array of comments made on the talk
 */
function joindin_get_ji_talk_comments_by_uri($comments_uri) {
  $result = NULL;
  $response = joindin_call_service_by_uri($comments_uri);
  if ($response
    && array_key_exists('comments', $response)
    && is_array($response['comments'])) {
    $result = $response['comments'];
  }
  else {
    $variables = array(
      '%uri' => $comments_uri,
    );
    watchdog('joindin', 'No valid commments data received for %uri', $variables);
  }
  return $result;
}

/**
 * @param int $ji_talk_id talk id on joind.in
 * @return mixed|null object representing the talk
 */
function joindin_get_ji_talk_comment($ji_comment_id) {
  return joindin_call_service("talk_comments/$ji_comment_id");
}

/**
 * Retrieves the event and all its talks from joindin and converts them to nodes
 *
 * @param int $ji_event_id
 * @return object|null newly saved node
 */
function joindin_create_event_node($ji_event_id) {
  $event_node = NULL;
  $result = NULL;
  // Get the event details from joind.in
  $ji_event = joindin_get_ji_event($ji_event_id);

  // if we got something back..
  if ($ji_event) {
    //..create and save a node for the event
    $event_node = new stdClass();
    $event_node->title = $ji_event->name;
    $event_node->type = 'joindin_event';
    $event_node->language = LANGUAGE_NONE;
    if (property_exists($ji_event, 'description')) {
      $event_node->body[$event_node->language][0]['value'] = $ji_event->description;
    }
    node_object_prepare($event_node);
    node_save($event_node);
    // todo should the event's host be the author?
    // note events can have multiple hosts, but we can always add these as a
    // string with all - an anonymous user is just a string after all
    // so could be "Submitted by Bill Birmingham and Brenda Brigstock (not verified) ..."
    // this would actually be the same for talks - talks can have multiple speakers

    // if save of event was successful..
    if ($event_node->nid) {
      // todo..add tags
      // ..add comments specific to this event..
      joindin_create_event_comments($event_node, $ji_event_id);

      // ..and get talks for the event
      joindin_create_event_talks($event_node, $ji_event_id);
    }
  }
  // only return the node if it saved properly
  if ($event_node != NULL && property_exists($event_node, 'nid')) {
    $result =  $event_node;
  }
  elseif ($ji_event) {
    $variables = array(
      '%ji_event_id' => $ji_event_id,
    );
    watchdog('joindin', "Could not save event node for event %ji_event_id", $variables);
  }
  return $result;
}

/**
 *
 *
 * @param $event_node object Drupal node to which to add comments
 * @param $ji_event_id int ID of the event in joind.in for which to retrieve comments
 */
function joindin_create_event_comments($event_node, $ji_event_id) {
  // todo when the node stores its ID, either as an int or url it won't need to be passed separately

  //  first get comments from the api..
  $ji_comments = joindin_get_ji_event_comments($ji_event_id);
  if ($ji_comments) {
    $publish = variable_get('joindin_publish_comments', TRUE);
    // ..then for each retrieved..
    foreach ($ji_comments as $ji_comment) {
      // add it to the event
      joindin_create_comment($event_node, $ji_comment, $publish);
    }
  }
}

/**
 * Creates a drupal node for the given joindin event
 *
 * @param $event_node object Drupal node to which to add comments
 * @param $ji_event_id int ID of the event in joind.in
 */
function joindin_create_event_talks($event_node, $ji_event_id) {
  // todo $event_node currently unused but will be when entity references are saved against the node
  // todo when the node stores its ID, either as an int or url it won't need to be passed separately
  $ji_talks = joindin_get_ji_event_talks($ji_event_id);
  // if the event has talks..
  if ($ji_talks) {
    //..for each talk..
    foreach ($ji_talks as $ji_talk) {
      //..populate and save a talk node
      $talk_node = new stdClass();
      $talk_node->title = $ji_talk['talk_title'];
      $talk_node->type = 'joindin_talk';
      $talk_node->language = LANGUAGE_NONE;
      if (array_key_exists('talk_description', $ji_talk)) {
        $talk_node->body[$talk_node->language][0]['value'] = $ji_talk['talk_description'];
      }
      node_object_prepare($talk_node);
      node_save($talk_node);
      // if we saved the talk successfully..
      if ($talk_node->nid) {
        //..get the comments made on this talk..
        if ($ji_talk['comments_uri']) {
          $ji_talk_comments = joindin_get_ji_talk_comments_by_uri($ji_talk['comments_uri']);
          if ($ji_talk_comments) {
            //..and for each..
            foreach ($ji_talk_comments as $ji_talk_comment) {
              // save the comment against the talk
              joindin_create_comment($talk_node, $ji_talk_comment);
            }
          }
        }
      }
    }
  }
}

/**
 * Saves the joindin comment response against the given node
 *
 * @param $node node to which to add comment
 * @param $ji_comment array with ji comment response
 * @param $publish bool whether comment should be automatically published
 */
function joindin_create_comment($node, $ji_comment, $publish) {
  // if they actually said anything..
  if (array_key_exists('comment', $ji_comment)) {
    //..convert to a Drupal comment on our new event node
    $comment = new stdClass();
    $comment->nid = $node->nid;
    $comment->cid = 0;
    $comment->pid = 0;  // joind.in doesn't thread comments
    if ($publish) {
      $comment->status = COMMENT_PUBLISHED;
    }
    $comment->language = LANGUAGE_NONE;
    $comment->comment_body[$comment->language][0]['value'] = $ji_comment['comment'];
    if (array_key_exists('user_display_name', $ji_comment)) {
      $comment->name = $ji_comment['user_display_name'];
    }
    $comment->date = $ji_comment['created_date'];
    comment_submit($comment);
    comment_save($comment);
    // todo what about ratings? soft dependency on voting api/fivestar?
  }
}
