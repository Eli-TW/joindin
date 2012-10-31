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
// todo also $string as a parameter name? rly?
function joindin_call_service($string) {
  // call service
  $result = NULL;
  $url = variable_get('joindin_base_url');
  $api_version = variable_get('joindin_api_version');
  // todo set header to accept json - or find out portable way to decode html in PHP
  $http_response = drupal_http_request("$url/$api_version/$string");
  // if our call was successful..
  if ($http_response->code == 200) {
    // ..decode and return the result
    $result = drupal_json_decode($http_response->data);
  }
  else {
    // todo better error message
    watchdog('joindin', "Call to joind.in failed: $url/$api_version/$string");
    // todo consider throwing exception?
  }
  return $result;
}

/**
 * @param $ji_event_id event id on joind.in
 * @return mixed|null array representing the event
 */
function joindin_get_ji_event($ji_event_id) {
  $response = joindin_call_service("/events/$ji_event_id");
  if (array_key_exists('events', $response)
    && array_key_exists(0, $response['events'])
    && array_key_exists('name', $response['events'][0])) {
    return (object)$response['events'][0];
  }
  else {
    return NULL;
  }
}

/**
 * @param $ji_event_id event id on joind.in
 * @return mixed|null object listing talks from that
 */
function joindin_get_ji_event_talks($ji_event_id) {
  return joindin_call_service("/events/$ji_event_id/talks");
}

/**
 * @param $ji_event_id event id on joind.in
 * @return mixed|null array listing comments for the given event
 */
function joindin_get_ji_event_comments($ji_event_id) {
  $response = joindin_call_service("/events/$ji_event_id/comments");
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
  return joindin_call_service("/talks/$ji_talk_id");
}

/**
 * @param $ji_talk_id talk id on joind.in
 * @return mixed|null object representing the talk
 */
function joindin_get_ji_talk_comments($ji_talk_id) {
  return joindin_call_service("/talks/$ji_talk_id/comments");
}

/**
 * @param $ji_talk_id talk id on joind.in
 * @return mixed|null object representing the talk
 */
function joindin_get_ji_talk_comment($ji_comment_id) {
  return joindin_call_service("/talk_comments/$ji_comment_id");
}

/**
 * @param $ji_event_id
 * @return newly saved node, or NULL
 */
function joindin_create_event_node($ji_event_id) {
  $node = NULL;
  // Get the event details from joind.in
  $ji_event = joindin_get_ji_event($ji_event_id);

  // if we got something back..
  if ($ji_event) {
    //..create and save a node
    $node = new stdClass();
    $node->title = $ji_event->name;
    $node->type = 'joindin_event';
    $node->language = LANGUAGE_NONE;
    if (property_exists($ji_event, 'description')) {
      $node->body[$node->language][0]['value'] = $ji_event->description;
    }
    node_object_prepare($node);
    node_save($node);
    // now add comments - first get them from the api..
    $ji_comments = joindin_get_ji_event_comments($ji_event_id);
    if ($ji_comments) {
      // ..then for each retrieved..
      foreach ($ji_comments as $ji_comment) {
        //..if they actually said anything..
        if (array_key_exists('comment', $ji_comment)) {
          //..convert to a Drupal comment on our new event node
          $comment = new stdClass();
          $comment->nid = $node->nid;
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
        }
      }
    }
  }
  // only return the node if it saved properly
  if ($node != NULL && property_exists($node, 'nid')) {
    return $node;
  }
  else {
    return NULL;
  }
}
