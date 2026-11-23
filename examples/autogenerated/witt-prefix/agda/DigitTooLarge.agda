{-# OPTIONS --safe --without-K #-}
module DigitTooLarge where

open import WittPrefix

outside-binary-range : Digit 2
outside-binary-range = later-digit (later-digit first-digit)
