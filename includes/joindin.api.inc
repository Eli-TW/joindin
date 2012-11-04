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
// todo abstract this once tested

/**
 * All calls to the joindin api go through this wrapper
 *
 * @param $path
 * path of the api call after the API version
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
    // todo better error message
    watchdog('joindin', "Call to joind.in failed: $url/$api_version/$path");
  }
  return $result;
}

/**
 * Retrieves event from joind.in
 *
 * @param $ji_event_id
 * event id on joind.in
 *
 * @return null|object
 * object representing event, or null if no reasonable response returned
 */

function joindin_get_ji_event($ji_event_id) {
  // ask joindin for the event..
  $response = joindin_call_service("events/$ji_event_id");
  //..if we received a response, containing an event..
  if ($response
    && array_key_exists('events', $response)
    && array_key_exists(0, $response['events'])
    && array_key_exists('name', $response['events'][0])) {
    //..return just that one event
    return (object)$response['events'][0];
  }
  else {
    //..we didn't get anything we can use
    return NULL;
  }
}

/**
 * @param $ji_event_id event id on joind.in
 * @return mixed|null object listing talks from that
 */
function joindin_get_ji_event_talks($ji_event_id) {
  // ask joindin for the talks for the specified event...
  $response = joindin_call_service("events/$ji_event_id/talks");
  //..if we received a response, containing an array of talks..
  if ($response
    && array_key_exists('talks', $response)
    && is_array($response['talks'])) {
    //..return those talks
    return $response['talks'];
  }
  else {
    //..we didn't receive a list of talks
    return NULL;
  }
}

/**
 * @param $ji_event_id event id on joind.in
 * @return mixed|null array listing comments for the given event
 */
function joindin_get_ji_event_comments($ji_event_id) {
  $response = joindin_call_service("events/$ji_event_id/comments");
  if (array_key_exists('comments', $response) && is_array($response['comments'])) {
    return $response['comments'];
  }
  else {
    return NULL;
  }
}

/**
 * @param $ji_talk_id talk id on joind.in
 * @return mixed|null object representing the talk
 */
function joindin_get_ji_talk($ji_talk_id) {
  return joindin_call_service("talks/$ji_talk_id");
}

/**
 * @param $ji_talk_id talk id on joind.in
 * @return mixed|null object representing the talk
 */
function joindin_get_ji_talk_comments($ji_talk_id) {
  return joindin_call_service("talks/$ji_talk_id/comments");
}

function joindin_get_ji_talk_comments_by_uri($comments_uri) {
  $response = joindin_call_service_by_uri($comments_uri);
  if ($response
    && array_key_exists('comments', $response)
    && is_array($response['comments'])) {
    return ($response['comments']);
  }
  else {
    return NULL;
  }
}


/**
 * @param $ji_talk_id talk id on joind.in
 * @return mixed|null object representing the talk
 */
function joindin_get_ji_talk_comment($ji_comment_id) {
  return joindin_call_service("talk_comments/$ji_comment_id");
}

/**
 * @param $ji_event_id
 * @return newly saved node, or NULL
 */
  function joindin_create_event_node($ji_event_id) {
  $event_node = NULL;
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

    // if save of event was successful..
    if ($event_node->nid) {
      // ..add comments specific to this event - first get them from the api..
      $ji_comments = joindin_get_ji_event_comments($ji_event_id);
      if ($ji_comments) {
        // ..then for each retrieved..
        foreach ($ji_comments as $ji_comment) {
          //..if they actually said anything..
          if (array_key_exists('comment', $ji_comment)) {
            //..convert to a Drupal comment on our new event node
            $comment = new stdClass();
            $comment->nid = $event_node->nid;
            $comment->cid = 0;
            $comment->pid = 0;  // joind.in doesn't thread comments
            $comment->status = COMMENT_PUBLISHED;
            $comment->language = LANGUAGE_NONE;
            $comment->comment_body[$comment->language][0]['value'] = $ji_comment['comment'];
            if (array_key_exists('user_display_name', $ji_comment)) {
              $comment->name = $ji_comment['user_display_name'];
            }
            // todo set $comment->created, it is sent by the API as 2012-10-07T11:15:08+01:00
            comment_submit($comment);
            comment_save($comment);
            // todo what about ratings? soft dependency on voting api/fivestar?
          }
        }
      }
      // now get talks for the event
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
            }
            if ($ji_talk_comments) {
              //..and for each..
              foreach ($ji_talk_comments as $ji_talk_comment) {
                //..if they actually said anything..
                if (array_key_exists('comment', $ji_talk_comment)) {
                  //..convert to a Drupal comment on our new event node
                  $talk_comment = new stdClass();
                  $talk_comment->nid = $talk_node->nid;
                  $talk_comment->cid = 0;
                  $talk_comment->pid = 0;  // joind.in doesn't thread comments
                  $talk_comment->status = COMMENT_PUBLISHED;
                  $talk_comment->language = LANGUAGE_NONE;
                  $talk_comment->comment_body[$comment->language][0]['value'] = $ji_talk_comment['comment'];
                  if (array_key_exists('user_display_name', $ji_talk_comment)) {
                    $talk_comment->name = $ji_talk_comment['user_display_name'];
                  }
                  // todo set $comment->created, it is sent by the API as 2012-10-07T11:15:08+01:00
                  comment_submit($talk_comment);
                  comment_save($talk_comment);
                  // todo what about ratings? soft dependency on voting api/fivestar?
                }
              }
            }
          }
        }
      }
    }
  }
  // only return the node if it saved properly
  if ($event_node != NULL && property_exists($event_node, 'nid')) {
    return $event_node;
  }
  else {
    return NULL;
  }
}
