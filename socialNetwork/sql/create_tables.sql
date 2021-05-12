-- Creation of product table
CREATE TABLE IF NOT EXISTS thrift_events (
  event_id integer NOT NULL,
  event_type smallint NOT NULL,
  logged_at bigint,
  sender_id varchar(250) NOT NULL,
  receiver_id varchar(250) NOT NULL,
  processed_count integer,
  PRIMARY KEY (event_id)
);