{-# OPTIONS --safe --without-K #-}
module WittFixtures where

open import Agda.Builtin.Equality using (_≡_; refl)
open import Agda.Builtin.Sigma using (_,_)
open import WittPrefix

binary-first : evaluate-ghosts two one-coordinate (later-digit first-digit) ≡ 1
binary-first = refl

binary-second :
  evaluate-ghosts two two-coordinates (later-digit first-digit , later-digit first-digit) ≡ (1 , 3)
binary-second = refl

binary-third-zero :
  evaluate-ghosts two three-coordinates
    ((later-digit first-digit , later-digit first-digit) , first-digit) ≡ ((1 , 3) , 3)
binary-third-zero = refl

binary-third-one :
  evaluate-ghosts two three-coordinates
    ((later-digit first-digit , later-digit first-digit) , later-digit first-digit) ≡ ((1 , 3) , 7)
binary-third-one = refl

ternary-two : Digit 3
ternary-two = later-digit (later-digit first-digit)

ternary-third-one :
  evaluate-ghosts three three-coordinates ((ternary-two , first-digit) , later-digit first-digit) ≡
  ((2 , 8) , 521)
ternary-third-one = refl

ternary-third-two :
  evaluate-ghosts three three-coordinates ((ternary-two , first-digit) , ternary-two) ≡ ((2 , 8) , 530)
ternary-third-two = refl

quinary-four : Digit 5
quinary-four = later-digit (later-digit (later-digit (later-digit first-digit)))

quinary-third-zero :
  evaluate-ghosts five three-coordinates ((quinary-four , first-digit) , first-digit) ≡
  ((4 , 1024) , 1125899906842624)
quinary-third-zero = refl

quinary-third-four :
  evaluate-ghosts five three-coordinates ((quinary-four , first-digit) , quinary-four) ≡
  ((4 , 1024) , 1125899906842724)
quinary-third-four = refl
