Rails.application.routes.draw do
  root "home#index"
  get 'admin', to: 'admin#index'
  get 'user', to: 'user#index'
  
  resources :pessoas
  resources :carros
  resources :computadors
  resources :disciplinas
  resources :disciplinas_pessoas
end

