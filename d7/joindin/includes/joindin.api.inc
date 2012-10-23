<?php
/**
 * Calls the joindin api
 *
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
    watchdog('joindin', 'Call to joind.in failed');
    // todo consider throwing exception?
  }
  return $result;
}

/**
 * @param $ji_event_id event id on joind.in
 * @return mixed|null object representing the event
 */
function joindin_get_ji_event($ji_event_id) {
  return joindin_call_service("/events/$ji_event_id");
}

/**
 * @param $ji_event_id event id on joind.in
 * @return mixed|null object listing talks from that
 */
function joindin_get_ji_event_talks($ji_event_id) {
  return joindin_call_service("/events/$ji_event_id/talks");
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
