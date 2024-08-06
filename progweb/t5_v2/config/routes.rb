Rails.application.routes.draw do
  root 'pages#home'
  get 'admin_view', to: 'pages#admin_view'
  get 'user_view', to: 'pages#user_view'

  namespace :admin do
    resources :pessoas, only: [:new, :create, :edit, :update, :destroy]
    resources :carros, only: [:new, :create, :edit, :update, :destroy]
    resources :computadors, only: [:new, :create, :edit, :update, :destroy]
    resources :disciplinas, only: [:new, :create, :edit, :update, :destroy]
    resources :dis_pes, only: [:new, :create, :edit, :update, :destroy]
  end
end

