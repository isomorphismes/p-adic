{-# OPTIONS --safe --without-K #-}
module MissingThirdGhost where

open import Agda.Builtin.Nat using (Nat)
open import Agda.Builtin.Sigma using (_,_; snd)
open import WittPrefix

third-value : Prefix Nat three-coordinates → Nat
third-value = snd

unavailable-value : Nat
unavailable-value = third-value (evaluate-ghosts two two-coordinates (first-digit , first-digit))
